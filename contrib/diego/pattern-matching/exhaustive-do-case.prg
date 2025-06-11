// the general idea is to have an exhaustive "match" or "do case"
// fsharp, ocaml, nim, typescript, smalltalk, eiffell and more are a great resources for improvements.
// here is a set of experiments using PP to achieve compile time error when a union type is not exhausted during a match.
// I've used the PP as factories to generate the valid commands, while invalidating the undesirable syntax.
// use cases for match: 
// it is common to have many cases or ifs spread over the code when dealing with something such as payment forms. and at the moment a new form is added, we often miss where to update it. This match solves this problem.
// for more, please read https://fsharpforfunandprofit.com

// references
// C:\harbour\contrib\xhb\cstruct.ch
// C:\harbour\include\hbclass.ch
// C:\harbour\include\std.ch
// https://fsharpforfunandprofit.com/posts/convenience-types/

#include "hbclass.ch"
#define EOC ;; // END OF COMMAND. ALLOWS TO HAVE MORE THAN ONE EXPRESSION ON INDIRECT # DIRECTIVES

#define false .f.
#define true .t.

// simple primitive string
CLASS STRING
    METHOD New()
    method type()
    method type_matches
ENDCLASS

method new() class STRING
return self

method type() class STRING
return "STRING"

method type_matches(t) class STRING
return "STRING" == t:type()



CLASS INTEGER
    METHOD New()
    method type()
    method type_matches
ENDCLASS

method new() class INTEGER
return self

method type() class INTEGER
return "INTEGER"

method type_matches(t) class INTEGER
return "INTEGER" == t:type()

// this pragma create classes using an F# dsl for type
// fsharp: type USAddress = {Street:string; City:string; State:string; Zip:string}
#xcommand TYPE <type_name> = <f1> of <t1> [, <f2> of <t2>] => ;
    CLASS <type_name>;;
        METHOD New(n) ;;
        method type();;
        method type_matches(t);;
        data object ;;
    ENDCLASS;;
        method New(n) class <type_name>;;
            logger("New", HB_JSONENCODE(n));;
                if empty(n) ;;
                    ::object := {;
                        "type" => <"type_name">, ;
                        "mutable" => false, ;
                        "fields" => ;
                                {;
                                <"f1"> => {"type" => <t1>():new():type(),"value" => nil};
                                [,<"f2"> => {"type" => <t2>():new():type(), "value" => nil}];
                                };
                    };;
                    else;;
                        ::object := {;
                            "type" => <"type_name">, ;
                            "mutable" => n\["mutable"\], ;
                            "fields" => ;
                                {;
                                <"f1"> => {"type" => <t1>():new():type(),"value" => n\["fields"\]\[<"f1">\]\["value"\]};
                                [,<"f2"> => {"type" => <t2>():new():type(), "value" => n\["fields"\]\[<"f2">\]\["value"\]}];
                                };
                    };;
                    end;;
        return self;;
    method type() class <type_name>;;
    return upper(<"type_name">) ;;
    method type_matches(t) class <type_name>;;
    return upper(<"type_name">) == t:type();;
    #xcommand let local \<v> of \<t> <f1> \<<f1>v1> [, <f2> \<<f2>v2>] => local \<v> := \<t>():new({"mutable" => false, "fields" => {<"f1"> => {"value" => \<<f1>v1>}[,<"f2"> => {"value" => \<<f2>v2>}]}})
    // other ideas for instantiating variables in a more controlable way. this would allow to create immutable variables for example
    // let creates immutable objects
    // #xcommand let local <v> of <t> => local <v> := <t>():new({"type" => "object","mutable" => false, ...})
    // #xcommand let private <v> of <t> => private <v> := <t>():new({"type" => "object","mutable" => false, ...})
    // #xcommand let <v> be <t> => <v> := <v>():set(<t>)
    // // var creates mutable objects
    // #xcommand var local <v> of <t> => local <v> := <t>():new({"type" => "object","mutable" => true, ...})
    // #xcommand var <v> be <t> => <v> := <t>

// UNION DSL factory + matcher DSL factory
// ==========================
#xcommand TYPE <type_name> = UNION <f1> of <t1> [, <f2> of <t2>] => ;
    CLASS <type_name>;;
        METHOD New() ;;
        method type();;
        data fields ;;
        data union ;;
    ENDCLASS;;
         method New() class <type_name>;;
             ::union := {{"field" => <"f1">, "type" => <t1>():new():type()} [,{"field" => <"f2">, "type" => <t2>():new():type()}]};;
         return self;;
    method type() class <type_name>;;
    return upper(<"type_name">)  ;;
    #xcommand let Matcher \<matcher_name> of <type_name> = match \<*x*> => #warning match is not exausted. union type: <type_name> ;;
    #xcommand let Matcher \<matcher_name> of <type_name> = match \<var_to_match> with WHEN <t1> -> \<caselambda<t1>> [, WHEN <t2> -> \<caselambda<t2>>] => ;
        function ___Matcher_\<matcher_name>(\<var_to_match>) EOC ;
        do case EOC;
            case <t1>():new():type_matches(\<var_to_match>) EOC;
                \<caselambda<t1>> ;
            [EOC case <t2>():new():type_matches(\<var_to_match>) EOC \<caselambda<t2>> ] EOC ; 
        end EOC ;
        return nil ;;
    #xcommand MATCH \<matcher_name> \<v> => ___Matcher_\<matcher_name>(\<v>)
// ==========================

// fsharp union type
// You can often use a discriminated union as a simpler alternative to a small object hierarchy. For example, the following discriminated union could be used instead of a Shape base class that has derived types for circle, square, and so on.
// fsharp syntax
// [ attributes ]
// type [accessibility-modifier] type-name =
//     | case-identifier1 [of [ fieldname1 : ] type1 [ * [ fieldname2 : ] type2 ...]
//     | case-identifier2 [of [fieldname3 : ]type3 [ * [ fieldname4 : ]type4 ...]
//     [ member-list ]

// fsharp example:
// type Address =
//    | US of USAddress
//    | UK of UKAddress

// code usage
// ==========================

type dinheiro = valor of INTEGER
type cartao = valor of INTEGER, csv of string, expiration of string
type pix = valor of INTEGER, data of STRING

type formas_de_pagamento = union dinheiro of dinheiro, cartao of cartao, pix of pix

let Matcher processar_formas_de_pagamento_matcher OF formas_de_pagamento = match pg with ;
    WHEN dinheiro -> dummy_processar_dinheiro(pg:object["fields"]["valor"]) , ;
    WHEN cartao -> dummy_processar_cartao(pg:object["fields"]["valor"], pg:object["fields"]["csv"], pg:object["fields"]["expiration"]), ;
    when pix -> dummy_processar_pix(pg:object["fields"]["valor"], pg:object["fields"]["data"])


let Matcher imprimir_formas_de_pagamento_matcher OF formas_de_pagamento = match pg with ;
    WHEN dinheiro -> dummy_imprimir_dinheiro(pg:object["fields"]["valor"]) , ;
    WHEN cartao -> dummy_imprimir_cartao(pg:object["fields"]["valor"], pg:object["fields"]["csv"], pg:object["fields"]["expiration"]), ;
    when pix -> dummy_imprimir_pix(pg:object["fields"]["valor"], pg:object["fields"]["data"])


// type declarations
// fsharp equivalent: type USAddress = {Street:string; City:string}
type USAddress = Street of STRING, City of STRING
// fsharp equivalent: type UKAddress = {Street:string; Town:string; PostCode:string}
type UKAddress = Street of STRING, Town of STRING, PostCode of STRING
// fsharp equivalent: type BRAddress = {Street:string; City:string}
type BRAddress = Street of STRING, City of STRING


// fsharp like DSL for union type
type Address = UNION US of USAddress, UK of UKAddress, BR of BRAddress

// fsharp like DSL for matcher
let Matcher AddressMatcher1 of Address = match address with ;
    WHEN USAddress -> qout("AddressMatcher1 This is a US Address"), ;
    WHEN UKAddress -> qout("AddressMatcher1 This is a UK Address"), ;
    WHEN BRAddress -> qout("AddressMatcher1 This is a BT Address")


let Matcher AddressMatcher2 of Address = match address2 with ;
    WHEN USAddress -> qout("AddressMatcher2 This is a US Address"), ;
    WHEN UKAddress -> qout("AddressMatcher2 This is a UK Address") // this line generates the compiler warning "W0001  match Address is not exausted."


procedure main()
    experiment_type()
    experiment_match()
return

procedure experiment_type()
    let local address of USAddress Street "101st Street", City "California"
    logger("experiment_type, type is:", address:type())
    logger("experiment_type, object is:", hb_jsonencode(address:object))
return

procedure experiment_match()
    let local address of USAddress Street "101st Street", City "California"
    let local mcartao of cartao valor 1000.01, csv "123", expiration "01/36"
    MATCH AddressMatcher1 address

    logger("experiment_match", HB_JSONENCODE(mcartao:object))
    MATCH processar_formas_de_pagamento_matcher mcartao
    MATCH imprimir_formas_de_pagamento_matcher mcartao

    // ideally, a compiler error should be raised when a type different than the defined on the matcher is passed here.
    // this should be illegal
    // MATCH AddressMatcher1 "a" 


return

static procedure logger(...)
qout("LOG:", procline(1), ...)
return

procedure dummy_processar_dinheiro(...)
    logger (procname(0), HB_JSONENCODE({...}))
return
procedure dummy_processar_cartao(...)
    logger (procname(0), HB_JSONENCODE({...}))
return
procedure dummy_processar_pix(...)
    logger (procname(0), HB_JSONENCODE({...}))
return

procedure dummy_imprimir_dinheiro(...)
    logger (procname(0), HB_JSONENCODE({...}))
return
procedure dummy_imprimir_cartao(...)
    logger (procname(0), HB_JSONENCODE({...}))
return
procedure dummy_imprimir_pix(...)
    logger (procname(0), HB_JSONENCODE({...}))
return
// ==========================





