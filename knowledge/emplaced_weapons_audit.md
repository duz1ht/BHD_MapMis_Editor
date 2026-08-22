# Auditoria de emplaced weapons e control registers

## Escopo e conclusão

> **Atualização da implementação:** o editor agora preserva todas as ocorrências
> de `addeweap*` em estruturas semânticas, lê os user points de 48 bytes, monta
> recursivamente os modelos-filhos nos anchors e cria um bus de 96 slots por
> instância com conversão/clamp de yaw e pitch. A decodificação da hierarquia de
> partes e a avaliação dos tracks `PANM` continuam pendentes porque exigem a
> validação do layout GPM/GPS/GPP descrita na Fase 0.

Esta auditoria compara o fluxo sugerido no guia de referência com o estado do
editor em `index.html`. O guia veio de outro projeto e, portanto, os offsets e
a organização em chunks não devem ser copiados sem validação binária contra os
arquivos de Black Hawk Down.

No momento da auditoria inicial, o editor **ainda não implementava emplaced
weapons como uma composição visual animada**. As lacunas encontradas foram:

- lê `addeweap*` apenas como uma chave genérica de `kv` em `itemsDef.js`;
- resolve e desenha somente o `graphic` do item colocado no MIS;
- pula os registros de 48 bytes no início do `.3di` sem expor os user points;
- não lê a tabela nominal de control registers nem os tracks de animação;
- achata os triângulos do `.3di` por material, perdendo a hierarquia de partes;
- não cria o item-filho indicado por `addeweap*`;
- não possui um bus de 96 registers nem uma avaliação de `PANM`.

Assim, os dados necessários existem no catálogo e nos assets, mas o pipeline
atual não consegue ligá-los. O trabalho deve começar pela descrição validada do
formato `.3di`, e não pelo cálculo de yaw/pitch.

## 1. Auditoria da implementação atual

### 1.1 Catálogo de itens

`itemsDef.js` é carregado antes da aplicação e convertido por `parseItemsDef`.
O parser aceita qualquer chave alfanumérica e guarda o restante inteiro como
uma string em `cur.kv`. Por isso uma linha como:

```text
addeweap ewep01 101876
```

fica disponível como:

```js
def.kv.addeweap === "ewep01 101876"
```

Isso é retenção acidental, não uma implementação da relação. Não há estrutura
para múltiplos attachments, classificação `addeweap`/`addeweapG`/`addeweapC`,
ID do filho, nome do anchor ou presença explícita dos quatro limites. Como uma
chave repetida sobrescreve a anterior em `kv`, definições com dois `addeweap`
(por exemplo, dois miniguns) perdem o primeiro attachment durante o parse.

Também há relações cujo ID-filho não está resolvido no catálogo primário
analisado. A resolução deve emitir diagnóstico e continuar desenhando o carrier,
em vez de falhar toda a cena.

**Estado:** parcial somente no nível lexical; sem modelo semântico.

### 1.2 Resolução de gráficos e criação da cena

`getRenderableGlbEntries` escolhe um único `graphic` para cada item do MIS. Em
seguida, `loadGraphicTemplate` carrega um `.3di` ou `.glb`, e a instância recebe
apenas posição, heading, pitch e escala do item-pai. Não existe passagem que
percorra attachments declarados pela definição, resolva o ID-filho e o anexe a
um anchor.

O cache atual é por nome de `graphic`. Isso pode continuar sendo usado para os
assets imutáveis, mas o estado de pose e o bus precisam pertencer à instância:
dois veículos que usam o mesmo template podem mirar em direções diferentes.

**Estado:** renderização do carrier implementada; composição carrier -> arma
ausente.

### 1.3 Registros de 48 bytes / user points

`parseThreeDi` começa os dados variáveis em `236 + value_at_0xb0 * 48`, mas
somente avança o cursor. Os registros não são retornados em `model`.

Uma inspeção dos assets de emplaced weapons confirma que esses registros têm
nomes típicos de pontos (`MFlash01`, `Bullet`, `BCasing`, `Camera` e `Usegun`) e
o layout de 48 bytes descrito no guia é compatível com essa evidência. Portanto,
o comentário do viewer auxiliar que chama essa região de “control registers” é
enganoso; no editor, ela deve ser tratada provisoriamente como tabela de user
points até que o layout seja coberto por fixtures.

Além de ler posição, direção, parte proprietária, tipo e nome, será necessário
reaplicar a matriz final da parte proprietária ao ponto. Isso é essencial tanto
para montar o filho em `ewep01` quanto para manter `MFlash01`, `Bullet` e câmera
acompanhando partes animadas.

**Estado:** tamanho conhecido e atualmente pulado; conteúdo não modelado.

### 1.4 Control registers e tracks

O editor não contém catálogo global de 96 slots, resolução nominal, constantes
para `EWEAP_GUNYAW`/`EWEAP_GUNPITCH`, nem avaliação dos controles `113..117`.
A busca nos assets de amostra confirma que modelos filhos como `em50cal.3di`,
`e50etek.3di`, `mk19.3di`, `minigun.3di` e `VBLtur.3di` contêm os nomes
`EWEAP_GUNYAW` e `EWEAP_GUNPITCH`; `minigun.3di` também contém `WEAP_SPIN`.

Contudo, os arquivos não apresentam necessariamente os literais ASCII `CTRL`,
`PANM` e `USRP`. Logo, a descrição em chunks do guia deve ser traduzida para a
serialização GPM/GPS/GPP real antes de implementar offsets. Os ordinais 55, 56
e 57 também devem ser verificados contra um catálogo/referência BHD, ainda que
sejam bons valores iniciais fornecidos pelo guia.

**Estado:** ausente; os assets demonstram demanda real.

### 1.5 Hierarquia e animação de partes

O parser preserva um número `group` em cada triângulo, mas
`createThreeDiObject` agrega triângulos apenas por material. A malha final não
mantém nós por parte, pivots, pais ou tracks. Mesmo que o bus fosse preenchido
agora, não haveria parte separada à qual aplicar yaw ou pitch.

Antes de `PANM`, o loader precisa produzir uma cena hierárquica estável. A
geometria deve continuar compartilhando materiais/texturas, mas vértices de
partes com transformações diferentes não podem ser fundidos no mesmo mesh.

**Estado:** geometria estática implementada; representação animável ausente.

### 1.6 Estado de mira no editor

O item MIS possui heading e pitch e o renderer sabe aplicar esses valores ao
modelo completo. Entretanto, não há simulação de ocupante/gunner, relação
carrier-gunner, estado de mira separado, limites de emplacement, heat ou spin.
O Event Editor conhece semanticamente a ação `Attach SSN To Emplaced Weapon`,
mas isso serve para editar/serializar a missão e não cria estado de runtime para
o preview 3D.

Para um editor de mapas, é preciso decidir explicitamente o produtor da pose:

1. **preview neutro:** bus zerado, arma centralizada;
2. **preview editável:** yaw/pitch de inspeção armazenados apenas na UI;
3. **preview derivado da missão:** simulação limitada de eventos/SSNs, muito
   mais complexa e inadequada como primeira entrega.

Recomenda-se começar pelas opções 1 e 2. Não se deve inferir que um evento de
attachment está ativo sem executar a lógica temporal da missão.

## 2. Mapa dos módulos relevantes

| Responsabilidade | Local atual | Situação |
|---|---|---|
| Fonte de `addeweap*` | `resources/BHD/items_lists/itemsDef.js` | Há relações reais, inclusive attachments repetidos. |
| Parse/merge do catálogo | `parseItemsDef`, `mergeItemCatalogDefs`, `applyDefs` em `index.html` | Preserva apenas a última ocorrência de cada chave. |
| Resolução item MIS -> definição | `getDefForItem` e `state.defsById` | Pode resolver o carrier e pode ser reutilizada para o ID-filho. |
| Seleção do asset | `getRenderableGlbEntries` e `loadGraphicTemplate` | Seleciona somente um graphic por item MIS. |
| Leitor binário `.3di` | `ThreeDiReader` e `parseThreeDi` | Lê geometria, texturas, materiais e parte dos grupos; pula user points. |
| Construção Three.js | `createThreeDiObject` | Achata por material; sem árvore de partes ou animações. |
| Transformação do carrier | `applyItemModelTransform` e `applyItemTransformToObject` | Aplica pose ao modelo completo. |
| Evento de emplaced weapon | `EVENT_ACTION_TYPES` e mapeamento `ATTACH_TO_EMPLACED_TO` | Suporte de edição MIS, não de preview/runtime. |
| Catálogo global CTRL | inexistente | Necessário. |
| Avaliador PANM | inexistente | Necessário. |
| Bus por instância | inexistente | Necessário. |

## 3. Plano de implementação proposto

### Fase 0 — fixtures e validação do formato (bloqueante)

1. Selecionar pares pequenos e representativos:
   - carrier `VBL01.3di` -> child `VBLtur.3di` via `VBLtur`;
   - um carrier com `ewep01` -> `em50cal.3di`;
   - carrier com dois anchors -> `minigun.3di`;
   - child com `WEAP_SPIN` -> `minigun.3di`.
2. Criar um inspecionador/teste que exponha offsets, contagens, user points,
   nomes de registers, partes, pivots, pais e tracks sem renderizar.
3. Documentar o layout GPM/GPS/GPP observado. Marcar campos ainda desconhecidos
   em vez de adaptar cegamente nomes/offsets do outro projeto.
4. Confirmar em fixtures que cada `control_param` local chega ao nome esperado e
   que os ordinais globais BHD são 55/56/57.

**Critério de aceite:** parse determinístico, bounds checking e snapshots dos
quatro casos acima, incluindo ordem local de CTRL diferente quando houver um
asset que demonstre isso.

### Fase 1 — modelo semântico de `addeweap*`

1. Adicionar `attachments: []` a cada definição.
2. No parser, reconhecer case-insensitivamente `addeweap`, `addeweapG` e
   `addeweapC` antes do armazenamento genérico em `kv`.
3. Guardar `{ kind, anchorName, childId, limitsPresent, down, up, right, left }`.
4. Preservar todas as ocorrências e distinguir limites ausentes de quatro zeros.
5. No merge entre catálogos, definir uma política explícita: a presença de
   attachments no suplemento substitui a lista base inteira; a ausência mantém
   a lista base. Evitar concatenar duplicatas silenciosamente.
6. Validar ID numérico, aridade e números de limites, gerando warnings com nome
   e ID da definição sem interromper o carregamento.

**Critério de aceite:** definições com dois attachments mantêm ambos; limites
ausentes e limites `0 0 0 0` produzem estruturas diferentes.

### Fase 2 — user points e árvore de partes `.3di`

1. Implementar `i32` no `ThreeDiReader` e parsear os registros de 48 bytes como
   fixed-point 16.16, mantendo também os valores raw.
2. Fazer lookup de nome case-insensitive, mas preservar o nome original para
   diagnóstico.
3. Decodificar a árvore de partes, pivots e parent indices do formato real.
4. Construir um `THREE.Group` por parte; agrupar geometria por `(parte,
   material)`, não só por material.
5. Representar user points como frames ligados à parte proprietária.

**Critério de aceite:** o filho acompanha corretamente um anchor estático e um
anchor ligado a uma parte cuja matriz é alterada em teste.

### Fase 3 — CTRL/PANM e bus global

1. Definir um catálogo imutável de 96 nomes/ordinais com lookup
   case-insensitive e constantes nomeadas, evitando números mágicos.
2. Parsear a tabela local de nomes conforme o layout BHD validado.
3. Parsear tracks e manter separados `localControlIndex` (diagnóstico) e
   `globalControlOrdinal` (avaliação), em vez de destruir o dado original.
4. Remapear apenas referências válidas; registrar nome desconhecido, índice
   fora de faixa e ordinal fora do bus.
5. Implementar primeiro o estilo 113 e testes da interpolação com aritmética
   assinada nos endpoints e valor unsigned no register. Só então adicionar
   114..117 segundo a semântica comprovada.
6. Avaliar a pose local ao redor do pivot e compor recursivamente com o pai.

**Critério de aceite:** uma fixture com ordem local invertida anima yaw e pitch
corretamente sem depender de `CTRL[0]`/`CTRL[1]`.

### Fase 4 — composição carrier -> filho

1. Para cada item renderizável, percorrer `def.attachments`.
2. Resolver `childId` por `state.defsById`, então resolver o `graphic` do filho.
3. Localizar `anchorName` no modelo-pai; montar a instância filha sob o frame do
   user point e a parte proprietária.
4. Detectar ciclos e impor profundidade máxima, pois a relação vem de conteúdo
   externo e pode formar cadeias inválidas.
5. Manter templates de assets em cache, mas clonar nós, materiais mutáveis,
   pose e bus por instância.
6. Fazer fallback gracioso: carrier continua visível e o log identifica
   definição, child ID, graphic ou anchor ausente.

**Critério de aceite:** carriers de um e dois attachments mostram a quantidade
correta de filhos nos anchors corretos, sem compartilhar pose entre instâncias.

### Fase 5 — preview de yaw/pitch e limites

1. Criar `Int32Array(96)` zerado por instância/avaliação.
2. Implementar helpers puros e testáveis para:
   - wrap circular;
   - `parent - gunner` (ou pose de preview equivalente);
   - clamp assimétrico de left/right e up/down;
   - conversão para fase unsigned de 16 bits.
3. Publicar somente nos ordinais semânticos, nunca em todos os índices locais.
4. Aplicar limites do attachment antes dos limites da arma. Como o catálogo
   atual ainda não modela limites próprios da arma, deixar essa segunda fonte
   como extensão explícita, não como zero implícito.
5. Inicialmente oferecer sliders de preview não persistentes e um botão de
   centralização. O MIS não deve ser alterado por inspecionar a arma.
6. Atualizar user points animados depois da pose das partes.

**Critério de aceite:** testes cobrem `0°`, `90°`, `180°`, `-90°`, travessia
`359°/1°`, clamps assimétricos e garantem que `-90°` chega ao bus como `49152`.

### Fase 6 — robustez, desempenho e UX

1. Avaliar PANM somente quando bus, tempo ou dependências da pose mudarem.
2. Liberar geometria, materiais e texturas de filhos ao reconstruir a cena.
3. Adicionar categorias de log `[emplaced]` e `[3di_animation]` com deduplicação
   por asset/erro para evitar milhares de mensagens.
4. Expor em modo diagnóstico: carrier, anchor, child ID/graphic, registers
   consumidos, limites efetivos e fases atuais.
5. Só depois considerar interpretar `Attach SSN To Emplaced Weapon` como
   runtime; isso exige estado temporal de eventos e não faz parte do preview
   estrutural de `addeweap*`.

## 4. Riscos e decisões abertas

- **Layout real:** a maior incerteza é a serialização de partes, tabela nominal
  e tracks nos GPM/GPS/GPP. Copiar offsets do guia pode gerar leituras válidas
  por acaso e corrupção silenciosa.
- **Unidades/eixos:** fixed-point, handedness, ordem de matrizes e sinal de
  yaw/pitch precisam ser testados junto à conversão de eixos já aplicada pelo
  renderer.
- **Limites próprios da arma:** é necessário identificar os nomes/campos reais
  no catálogo BHD; ausência não pode virar clamp zero.
- **Semântica G/C:** preservar `kind` desde o primeiro parse, mesmo que a UI não
  use a classificação ainda.
- **GLB:** um `.glb` convertido pode não preservar metadata de user points ou
  PANM. Attachments animados devem preferir `.3di` ou exigir metadata equivalente
  na conversão.
- **Gunner:** `addeweap*` define a composição estrutural, não identifica sozinho
  quem está operando a arma. Preview e simulação de missão são escopos distintos.

## 5. Ordem recomendada de entrega

1. Fase 0 (formato e fixtures).
2. Fase 1 (`addeweap*` estruturado, incluindo repetições e limites).
3. Fase 2 (user points + hierarquia).
4. Fase 4 parcial (filho centralizado no anchor, ainda sem animação).
5. Fase 3 (CTRL/PANM).
6. Fase 5 (bus e preview de pose).
7. Fase 6 (robustez e diagnóstico).

Essa sequência produz valor visual cedo, mas evita uma implementação que
“parece funcionar” ao escrever yaw/pitch em índices locais incorretos ou ao
animar a geometria do carrier quando o consumidor é o modelo-filho.
