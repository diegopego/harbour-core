# Incremental Lexer & Macro Expander Prototype

## Objetivos

- Fornecer tokenização incremental com preservação de ranges para texto original e resultado pós-expansão.
- Permitir reprocessamento seletivo quando um trecho do arquivo muda, evitando reconstruir todo o AST.
- Instrumentar o pré-processador para registrar traços de expansão (call stack, argumentos, simbolos).

## Requisitos Funcionais

1. Produzir fluxo de tokens estável (`stable token id`) que sobreviva a edições menores.
2. Emitir metadados essenciais por token:
   - `source_range_original`
   - `source_range_expanded`
   - `macro_origin` (quando aplicável)
   - `channel` (código, comentário, diretiva, trivia)
3. Gerenciar includes condicionais e diretivas de compilação preservando o contexto de ativação.
4. Suportar lexing incremental:
   - cache por bloco lógico (ex.: função, procedimento, bloco de macro).
   - estratégia _rope_ ou peças (`piece table`) para remapeamento de offsets.

## Arquitetura Proposta

```
        fonte.prg
            │
            ▼
    SourceBuffer (piece table)
            │
            ▼
 IncrementalScanner ──► TokenCache
            │                │
            ▼                ▼
  MacroResolver        ExpansionTraceLog
            │                │
            └────────────► TokenStream (com dual ranges)
```

- **SourceBuffer**: abstrai texto com suporte a inserções/deleções; fornece iteradores rápidos.
- **IncrementalScanner**: detecta blocos invalidados com base em dirty ranges e retokeniza apenas o necessário.
- **MacroResolver**: aplica expanders, mantendo stack de invocação para navegação inversa.
- **ExpansionTraceLog**: grafo direcionado armazenando relações `macro -> expansão`.
- **TokenStream**: sequência final usada pelo parser, expõe _snapshots_ para agents externos.

## Estruturas de Dados

- `TokenId`: hash estável de (`file_id`, `range_original`, `token_kind`, `macro_depth`).
- `TokenEntry`:
  ```
  struct TokenEntry {
      TokenId id;
      Range original;
      Range expanded;
      TokenKind kind;
      uint8_t channel;
      MacroCallRef origin;
  };
  ```
  - `origin` encapsula nome da macro, módulo, intervalo e identificador determinístico da invocação atual (`hb_astMacroTraceId()`), acessíveis via helpers `hb_astMacroTrace*()`; cada nó mantém ponteiro para o pai para reconstruir a pilha e pode ser enumerado pelo snapshot (`hb_astTokenStreamMacroTrace*`).
- `ExpansionNode`: descreve cada expansão de macro com ponteiros para tokens de entrada/saída.
- `ScopeGuard`: acompanha diretivas condicionais (`#ifdef`, `#endif`) e produz mapa de regiões ativas.

## Plano de Implementação

1. **Refatorar pré-processador atual** para separar leitura de arquivo, tokenização e expansão.
2. **Introduzir camada de buffer mutável** utilizando `hb_xxx` utilitários ou adaptação de peça existente.
3. **Implementar cache de tokens por intervalo**:
   - mapear offset lógico para blocos (`TokenBlock`).
   - invalidar blocos afetados por edições; reaproveitar demais.
4. **Registrar traço de expansão**:
   - push/pop de `MacroCall` em uma pilha global.
   - persistir grafo em formato compacto (ex.: adjacency list).
5. **Expor API pública**:
   - `hb_ast_lex(snapshot_config)` retorna `TokenStreamView`.
   - `hb_ast_trace(token_id)` retorna cadeias de macro até a fonte original.
6. **Integração com parser**:
   - adaptar parser para consumir `TokenStreamView`.
   - garantir que `stable ids` sejam propagados aos nós do AST.

## Validação

- **Testes de mutação**: alterar um arquivo em pontos distintos (início, meio, fim) e validar o número de blocos reprocessados.
- **Round-trip de macro**: garantir que rename em símbolo expandido reflita corretamente no local original.
- **Cobertura de dialetos**: incluir testes com Harbour puro, extensões xHarbour e macros complexas (`TEXT...ENDTEXT`, `#command`).
- **Desempenho**: coletar métricas de tempo antes/depois e estabelecer baseline para regressão.

## Entregáveis

- Código experimental em `src/ast/lexer/` (protótipo separado do pipeline de produção).
- Conjunto de testes em `tests/ast/incremental_lexer/`.
- Documentação de API em `doc/agents/ast/incremental-lexer.md` (este arquivo) e diagrama atualizado em `Agents.md`.

## Questões em Aberto

- Estratégia para sincronizar `TokenId` entre builds em diferentes plataformas (normalização de paths/encodings).
- Persistência do `ExpansionTraceLog`: arquivo separado ou embutido na serialização do AST?
- Níveis de macro (pré-processamento vs runtime `&`): necessidade de modelo híbrido?
