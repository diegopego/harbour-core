# `hbast verify` Prototype

## Propósito

Validar artefatos AST exportados garantindo conformidade com:

1. Esquema JSON (`hbast.schema.json`) e, futuramente, payload CBOR equivalente.
2. Regras de integridade adicionais (IDs únicos, ranges coerentes, referências válidas).
3. Convenções de versionamento (`format_version`, `schema_revision` compatíveis com o verificador).

## Requisitos Funcionais

- Aceitar arquivos JSON (`.hbast.json`) e CBOR (`.hbast.cbor`).
- Permitir leitura via STDIN (`hbast verify -`) para pipelines.
- Expor opções:
  - `--schema <path>`: usar schema customizado.
  - `--strict`: promover avisos a erros.
  - `--summary`: emitir apenas relatório final (sucesso/falhas).
- Retornar código de saída `0` quando não houver violações fatais; `1` quando falhar; `2` para erros de execução.

## Fluxo de Execução

1. **Detecção de formato**: heurística pelo cabeçalho e/ou extensão.
2. **Carregamento**:
   - JSON: parser `hb_jsonLoad`.
   - CBOR: usar backend `hb_cborRead` (a desenvolver).
3. **Validação de schema**:
   - Carregar `hbast.schema.json`.
   - Executar motor de validação (novo módulo `hbjsonschema` ou integração existente).
4. **Regras adicionais**:
   - IDs (`node.id`, `symbol.symbol_id`, `token.id`) devem ser únicos.
   - `stable_id` precisa ser determinístico (regex `.+:.+:.+@.+`).
   - `children`/`parent` consistentes (grafo sem ciclos).
   - Referências (`references`, `output_tokens`, `token`) não podem apontar fora do array.
   - Ranges: `start <= end`, offsets dentro do limite do arquivo.
5. **Relatório**:
   - Detalhar falhas, indicando caminho JSON (`files[0].ast.nodes[2].children[5]`).
   - Emitir estatísticas (tempo total, arquivos processados, contagem de tokens/nós).
   - Opção `--summary` limita a linha final `hbast verify: OK (n files)` ou `hbast verify: FAILED (x issues)`.

## Estrutura de Código Proposta

```
src/ast/verify/
 ├─ hbast_verify.c      (CLI principal e parsing de argumentos)
 ├─ hbast_schema.c      (wrapper para validação via JSON Schema)
 ├─ hbast_integrity.c   (regras de consistência específicas)
 └─ hbast_cbor.c        (decodificador CBOR opcional)
```

- Dependências reutilizadas: `hbjson`, `hbzlib` (para compressão opcional).
- APIs planejadas:
  - `HB_AST_VERIFY_RESULT hb_astVerifyRun( const HB_AST_VERIFY_CONFIG * )`
  - `HB_BOOL hb_astIntegrityCheck( const HB_JSON * pRoot, HB_AST_DIAGNOSTIC_LOG * pLog )`

## Saída de Diagnósticos

- Formato padrão:
  ```
  <severity>:<code> <message>
      at <json pointer>
      hint: <suggestion>
  ```
- Severidades: `ERROR`, `WARNING`, `INFO`.
- Em modo `--strict`, avisos contam como falha.

## Marcos

1. **MVP**: leitura JSON + validação de schema + relatório textual.
2. **Integração CBOR**: adicionar suporte de leitura e comparação JSON↔CBOR.
3. **Testes automatizados**: fixtures em `tests/ast/verify` com casos válidos e inválidos.
4. **Integração CI**: alvo `make verify-ast` executando o comando para PRs que tocarem arquivos `.hbast`.
