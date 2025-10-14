# Harbour AST Serialization Specification (Draft)

## Objetivos

- Definir representação estável da AST para consumo por ferramentas externas e agentes internos.
- Oferecer versão textual (JSON) e binária compacta (CBOR) com paridade semântica.
- Permitir _streaming_ e leitura parcial para suportar pipelines incrementais.

## Princípios

- **Versionamento explícito**: cada payload começa com `format_version`, `schema_revision` e `generator`.
- **IDs estáveis**: nós reutilizam `stable_id` composto (`file`, `offset`, `kind`, `hash_context`).
- **Separação de concern**: metadados ricos (ex.: docstring, comentários) via canais opcionais.
- **Extensibilidade**: novos campos introduzidos via `features` e `capabilities` negociados.

## Estrutura de Topo

```json
{
  "format_version": "0.1.0",
  "schema_revision": 1,
  "generator": {
    "name": "hbast",
    "version": "0.1.0",
    "build": "2024-05-15"
  },
  "project": {
    "root": "/abs/path",
    "dialect": "harbour",
    "flags": ["legacy-macros", "strict-semantics"]
  },
  "files": [ /* FileEntry[] */ ],
  "symbols": [ /* SymbolEntry[] */ ],
  "diagnostics": [ /* DiagnosticEntry[] */ ]
}
```

### `FileEntry`

```json
{
  "file_id": 1,
  "path": "src/foo.prg",
  "hash": "sha256:...",
  "ast": {
    "root": 1001,
    "nodes": [ /* NodeEntry[] */ ]
  },
  "macros": {
    "expansions": [ /* MacroExpansion[] */ ]
  },
  "includes": [2, 3],
  "metadata": {
    "line_endings": "lf",
    "encoding": "utf-8"
  }
}
```

### `NodeEntry`

```json
{
  "id": 1001,
  "kind": "FunctionDecl",
  "range": {
    "start": {"line": 10, "column": 1},
    "end": {"line": 30, "column": 8}
  },
  "stable_id": "foo.prg:10:FunctionDecl@a1b2",
  "parent": 0,
  "children": [1002, 1003],
  "tokens": [2001, 2002],
  "symbol": 5001,
  "type": {"hint": "FUNCTION", "confidence": 0.8},
  "attributes": {
    "visibility": "export",
    "async": false
  },
  "doc": 9001
}
```

### `SymbolEntry`

```json
{
  "symbol_id": 5001,
  "kind": "Function",
  "name": "MyProc",
  "qualified_name": "MyNamespace.MyProc",
  "declarations": [1001],
  "references": [3001, 3002],
  "scope": {
    "parent": 4001,
    "kind": "Module"
  },
  "annotations": ["deprecated:false"]
}
```

### `MacroExpansion`

```json
{
  "expansion_id": 6001,
  "macro_name": "DBG",
  "definition_range": {"file": 2, "range": {/* ... */}},
  "call_site": {"file": 1, "token": 2101},
  "arguments": ["expr", "message"],
  "output_tokens": [2105, 2106]
}
```

### `DiagnosticEntry`

```json
{
  "severity": "warning",
  "code": "HB0001",
  "message": "Suspicious implicit conversion",
  "range": {"file": 1, "start": {"line": 12, "column": 5}, "end": {"line": 12, "column": 14}},
  "related": [
    {"file": 1, "range": {/* ... */}, "message": "Assigned here"}
  ],
  "fixes": [
    {
      "title": "Add explicit conversion",
      "edits": [
        {"file": 1, "range": {/* ... */}, "text": "HB_Int("}
      ]
    }
  ]
}
```

## Canalização de Dados

- **Tokens**: exportados em canal separado opcional (`token_stream`). Referenciados por `TokenRef`.
- **Comentários & Trivia**: armazenados em `channels.commentary` para permitir reconstrução fiel.
- **Semântica**: grafo de dependência via `symbols` + `references`.

## CBOR Mapping

- Seguir RFC 7049. Campos repetidos mapeados para _arrays_ ou _maps_ com `unsigned int` chave.
- Strings de caminho codificadas como `text`. IDs preferem `uint`.
- Permitir _tags_ personalizados para `Range` (`tag 1000`) e `TokenId` (`tag 1001`) para reduzir tamanho.

## Versionamento & Evolução

1. Incrementar `schema_revision` quando campos obrigatórios mudarem ou forem removidos.
2. Compatibilidade retroativa mantida por pelo menos duas revisões.
3. Clientes devem anunciar `required_capabilities` ao solicitar dados via LSP/IPC.

## Segurança & Integridade

- Hash por arquivo (SHA-256) e do payload total.
- Assinatura opcional para pipelines sensíveis.
- Validar limites para evitar _resource exhaustion_ em arquivos maliciosos.

## Próximos Passos

- Especificar gramática JSON Schema (`hbast.schema.json`).
- Criar protótipo CBOR com biblioteca `zlib` ou similar para compressão opcional.
- Integrar `hbast verify` para validar payloads contra schema.
