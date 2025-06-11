// experimenting with the "restricted match marker"
// =================
// #xcommand ALIGN TO [<alinhamento:center,left>] => alinha(<alinhamento>)
// function teste_alinha()
//     align to CENTRO
// return nil
// =================


// experimenting with inject indirect # directives in any place
// =================
// #xcommand CREATECMD     => #xcommand NEWCMD => QOut("1") EOC QOut("2")
// procedure a()
//     CREATECMD // #xcommand NEWCMD => QOut("1") EOC QOut("2")
//     NEWCMD // QOut("1") ; QOut("2")
// return

// #xcommand CREATECMD     => local b EOC b := "b variable" EOC QOut(b) EOC #xcommand NEWCMD => QOut("1") EOC QOut("2")
// procedure a()
//     CREATECMD // #xcommand NEWCMD => QOut("1") EOC QOut("2")
//     NEWCMD // QOut("1") ; QOut("2")
// return
// =================



// ===========================
// experiment with matching
// this works to invalidate any let Matcher Address = match.
// I hope that this will give me a better control over the compiler error messages.
// #xcommand let Matcher <type_name> = match <*x*> => #warning match <type_name> is not exausted.

// let Matcher Address = match address with ;
//     WHEN USAddress -> qout("This is a US Address"), ;
//     WHEN UKAddress -> qout("This is a UK Address"), ;
//     WHEN BRAddress -> qout("This is a BT Address")
// ===========================

// ===========================
// experiment with matching
// #xcommand let Matcher Address = match <var_to_match> with WHEN USAddress -> <caselambdaUSAddress>, WHEN UKAddress -> <caselambdaUKAddress>, WHEN BRAddress -> <caselambdaBRAddress> => procedure experiment() ; qout("hello") ; return
//
// let Matcher Address = match address with ;
//     WHEN USAddress -> qout("This is a US Address"), ;
//     WHEN UKAddress -> qout("This is a UK Address"), ;
//     WHEN BRAddress -> qout("This is a BT Address")
// ===========================

// ===========================
// experiment with matching
// #xcommand let Matcher Address = match <var_to_match> with WHEN <v> -> <l> [,WHEN <v> -> <l>] => procedure experiment() ; qout("hello") ; return
//
// let Matcher Address = match address with ;
//     WHEN USAddress -> qout("This is a US Address"), ;
//     WHEN UKAddress -> qout("This is a UK Address"), ;
//     WHEN BRAddress -> qout("This is a BT Address")
// ===========================


// ===========================
// gives syntax error if using an invalid when clause but it is not exhaustive.
// #xcommand let Matcher Address = match <var_to_match> with WHEN <when_clause:USAddress,UKAddress,BRAddress> -> <l> [,WHEN <when_clause_2:USAddress,UKAddress,BRAddress> -> <l_2>] => procedure experiment() ; qout("hello") ; return
// let Matcher Address = match address with ;
//     WHEN USAddress -> qout("This is a US Address"), ;
//     WHEN UKAddress -> qout("This is a UK Address")
// ===========================


// // ciclyc <@> directive experiment
// // Ele avisa via em tempo de compilaá∆o ao realizar um case inesperado.
// function compiler_should_raise_error_on_invalid_case(employee)
//     // por brevidade, n∆o criei as diretivas pra o match, mas aqui estou simulando:
//     // match my_payment as PAYMENT. isso criaria as seguintes diretivas:
//     #command case <var1> => ;
//         #warning "invalid" ; <@> case <var1>
//     // aqui faáo a suposiá∆o que typeof usa string para verificar o tipo, mas isso pode ser usado de outras formas, incluindo invocando a classe. ex.: Money():new():type
//     #command case MONEY [<var1>] => ;
//             DECLARED case typeof(my_payment,"MONEY") [<var1>]
//     #command case CREDIT_CARD [<var1>] => ;
//             DECLARED case typeof(my_payment,"CREDIT_CARD") [<var1>]
//     #command DECLARED case [<var1>] => ;
//             <@> case [<var1>]
//     do case
//         case MONEY
//             ? "MONEY"
//         case MONEY .and. my_payment:value > 0
//             ? "MONEY .and. my_payment:value > 0"
//         case INVALID_PAYMENT
//             // should render compiler error
//         case CREDIT_CARD .and. my_payment:value > 0
//             ? "CREDIT_CARD .and. my_payment:value > 0"
//     endcase
// return nil
// // endregion recursividade, referància c°clica, referencia ciclica