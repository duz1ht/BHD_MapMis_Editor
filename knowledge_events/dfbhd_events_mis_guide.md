# DFBHD MED Events: guia de leitura e escrita do .mis

Este guia foi montado a partir de análise estática do `dfbhdmed.exe` enviado e cruzamento com o mapa `.mis` enviado. O objetivo é reproduzir a janela Events em um novo editor sem perder dados de mapas existentes.

## 1. O ponto mais importante

Um Event no `.mis` não é salvo como uma expressão textual do tipo IF/THEN. Ele é um bloco numerado com metadados, uma lista de Triggers e uma lista de Actions. Cada Trigger e cada Action é serializado como exatamente 8 inteiros decimais com sinal.

Exemplo estrutural:

```text
begin event N
  name "Event N"
  attrib X
  reset_value X
  delay_value X
  begin triggers C
    a b c d e f g h
  end triggers
  begin actions C
    a b c d e f g h
  end actions
end event
```

O MED omite `attrib`, `reset_value`, `delay_value`, `begin triggers` e `begin actions` quando seus valores/contagens são zero. `name` é sempre gravado. O cabeçalho do arquivo também contém `num_events`, que precisa acompanhar a quantidade real de Events.

## 2. Estrutura do Event

| Campo no .mis | Controle da janela | Regra |
|---|---|---|
| `name` | Name | Nome do Event. A UI deste MED trabalha com aproximadamente 59 caracteres visíveis. |
| `attrib` bit `0x01` | Reset after (seconds) | Liga o reset periódico. |
| `attrib` bit `0x02` | Pre Mission Event | Marca como Pre Mission Event. |
| `attrib` bit `0x04` | Post Mission Event | Marca como Post Mission Event. |
| `reset_value` | Reset after (seconds), valor | Número digitado no campo de segundos. |
| `delay_value` | Delay (seconds) | Atraso do Event. |
| `begin triggers N` | IF (Triggers) | Contagem e linhas de Trigger. |
| `begin actions N` | THEN (Actions) | Contagem e linhas de Action. |

### Regra crítica para `attrib`

A janela Events só altera os bits `0x01`, `0x02` e `0x04`. Outros bits são preservados. No mapa enviado existe, por exemplo, `attrib 48` (`0x30`) no Event 143. Seu editor não deve reconstruir `attrib` somente a partir dos três checkboxes, porque isso apagaria bits que o MED original preserva.

Use algo equivalente a:

```text
attribNovo = attribOriginal
set_or_clear(attribNovo, 0x01, resetAfterChecked)
set_or_clear(attribNovo, 0x02, preMissionChecked)
set_or_clear(attribNovo, 0x04, postMissionChecked)
```

## 3. Trigger: ordem exata dos 8 inteiros no .mis

```text
flags  trigger_type  condition  var1  var2  var3  var4  reserved7
```

### `flags`

| Bit | Significado |
|---|---|
| `0x01` | Se ligado, Trigger if = False. Se desligado, True. |
| `0x02` | Logic = OR. |
| `0x04` | Logic = XOR. |
| nenhum `0x02/0x04` | Logic = AND. |

Valores comuns: `0=True AND`, `1=False AND`, `2=True OR`, `3=False OR`, `4=True XOR`, `5=False XOR`. Preserve bits desconhecidos e `reserved7`.

### Trigger Type

| ID | Nome interno | Selecionável nesta versão do MED? |
|---:|---|---|
| 0 | `NULL` | Sim |
| 1 | `GROUP` | Sim |
| 2 | `SINGLE` | Sim |
| 3 | `EVENT` | Sim |
| 4 | `MISSION_VARIABLE` | Sim |
| 5 | `SECOND_TIME` | Sim |
| 6 | `TEAMMATES` | Sim |
| 7 | `PLAYER` | Não |

`PLAYER` (7) é reconhecido pelo executável, mas não aparece na lista Trigger Type desta versão. O mapa enviado usa `PLAYER` em 3 Triggers, todos com condição 36.

### Conditions de GROUP e SINGLE

| ID | GROUP | SINGLE | Variáveis principais | Aparece na lista desta versão? |
|---:|---|---|---|---|
| 0 | `NULL` | `NULL` | - | Sim |
| 1 | `SAW_GROUP` | `SAW_GROUP` | GROUP1, GROUP2 | Não |
| 2 | `TARGETED_GROUP` | `TARGETED_GROUP` | GROUP1, GROUP2 | Sim |
| 3 | `CONDITION_RED` | `CONDITION_RED` | GROUP1 | Sim |
| 4 | `ALL_DESTROYED` | `DEAD` | GROUP1 | Sim |
| 5 | `ANY_ALIVE` | `ALIVE` | GROUP1 | Sim |
| 6 | `LOST_X_UNITS` | `LOST_X_HP` | GROUP1, NUM_UNITS | Sim |
| 7 | `REACHED_WAYPOINT` | `REACHED_WAYPOINT` | GROUP1, WAYPOINT_LIST, WAYPOINT_NUMBER | Sim |
| 8 | `UNKNOWN` | `UNKNOWN` | - | Não |
| 9 | `ALL_INTACT` | `FULL_HP` | GROUP1 | Sim |
| 10 | `AREA_TRIGGER` | `AREA_TRIGGER` | GROUP, AREA | Sim |
| 11 | `HOLDS_ITEM` | `HOLDS_ITEM` | GROUP, ITEM_GROUP | Sim |
| 12 | `HAS_X_UNITS` | `HAS_X_HP` | GROUP1, NUM_UNITS | Sim |
| 13 | `SHOT_GROUP` | `SHOT_GROUP` | GROUP1, GROUP2 | Sim |
| 14 | `CONDITION_YELLOW` | `CONDITION_YELLOW` | GROUP1 | Sim |
| 15 | `TARGETED_SINGLE` | `TARGETED_SINGLE` | GROUP1, SINGLE2 | Sim |
| 16 | `SAW_SINGLE` | `SAW_SINGLE` | GROUP1, SINGLE2 | Não |
| 17 | `SHOT_SINGLE` | `SHOT_SINGLE` | GROUP1, SINGLE2 | Sim |

Para SINGLE, os nomes de Var mudam para SINGLE1/SINGLE2 quando aplicável. A definição completa está no JSON de schema que acompanha este guia.

### MISSION_VARIABLE

| Condition | Nome | Vars |
|---:|---|---|
| 0 | `NULL` | `MISSION_VARIABLE`, `VALUE` |
| 1 | `EQUALS` | `MISSION_VARIABLE`, `VALUE` |
| 2 | `LESS_THAN` | `MISSION_VARIABLE`, `VALUE` |
| 3 | `GREATER_THAN` | `MISSION_VARIABLE`, `VALUE` |
| 4 | `LESS_THAN_EQUAL` | `MISSION_VARIABLE`, `VALUE` |
| 5 | `GREATER_THAN_EQUAL` | `MISSION_VARIABLE`, `VALUE` |

True/False inverte o teste. Por exemplo, `False + EQUALS` representa a ideia de "não é igual".

### EVENT

`trigger_type = 3`, `condition = 0`, `var1 = índice do Event`. True significa Event Triggered e False significa Event Not Triggered. Como o índice é posicional, mover/inserir/apagar Events exige atualizar esta referência.

### SECOND_TIME

`trigger_type = 5`, normalmente `condition = 0`, sem Vars. True/False alterna a lógica de segunda passagem.

### TEAMMATES

| Condition | Nome | Selecionável nesta versão? |
|---:|---|---|
| 0 | `NULL` | Sim |
| 1 | `TEAMMATE_ENABLED` | Não |
| 2 | `TEAMMATE_MEDIC` | Não |
| 3 | `TEAMMATE_EVAC` | Sim |

### PLAYER

| Condition | Nome | Vars |
|---:|---|---|
| 0 | `NULL` | - |
| 18 | `PLAYER_BERSERK_TEST` | - |
| 19 | `PLAYER_1STPERSON_TEST` | - |
| 20 | `PLAYER_3RDPERSON_TEST` | - |
| 21 | `PLAYER_COCKPITVIEW_TEST` | - |
| 34 | `PLAYER_DLGDONE_TEST` | WAV_LIST_NUMBER |
| 35 | `PLAYER_DLGFINISHED_TEST` | WAV_LIST_NUMBER |
| 36 | `PLAYER_AWOL_TEST` | TIME_SEC |
| 37 | `PLAYER_SATCHEL_TEST` | AREA |

Todo o tipo PLAYER é oculto no dropdown desta versão do MED, mas precisa ser lido e preservado pelo novo editor.

## 4. Action: ordem exata dos 8 inteiros no .mis

Esta é a parte mais fácil de interpretar errado. A ordem serializada no arquivo é:

```text
reserved0  action_type  var1  var2  var3  var4  subtype_or_ai_action  reserved7
```

Internamente, o MED mantém `subtype_or_ai_action` antes das Vars na struct de memória, mas o writer do `.mis` move esse valor para o sétimo inteiro da linha. Para o parser do arquivo, use a ordem acima.

Exemplo real:

```text
0 3 30 25 -1 -1 30 0
```

Decodificação: Action Type 3 = `CHANGE_GROUP_AI`; Var1 = Group 30; Var2 = 25; subtipo 30 = `PATROLSPEED`.

### Action Type completo conhecido pelo executável

| ID | Nome | Vars principais | Selecionável nesta versão? |
|---:|---|---|---|
| 0 | `NULL` | - | Sim |
| 1 | `REDIRECT_GROUP_TO` | GROUP, WAYPOINT_LIST, WAYPOINT_NUMBER | Sim |
| 2 | `KILL_GROUP` | GROUP | Sim |
| 3 | `CHANGE_GROUP_AI` | GROUP, AI_PARAM_1, AI_PARAM_2, AI_PARAM_3 | Sim |
| 4 | `VAPORIZE_GROUP` | GROUP | Sim |
| 5 | `MISVAR_CHANGE` | MISSION_VARIABLE, VALUE | Sim |
| 6 | `OUTPUT_TEXT` | TEXT_ID | Não |
| 7 | `PLAY_WAVLIST` | WAV_LIST_NUMBER, PLAY_AFTER_END | Não |
| 8 | `BLUE_WIN` | - | Sim |
| 9 | `RED_WIN` | - | Sim |
| 10 | `GREEN_WIN` | - | Não |
| 11 | `GROUP_VELOCITY` | GROUP, VELOCITY_KPH | Não |
| 12 | `AREA_AI_RED` | AREA_ID, AI_PARAM_1, AI_PARAM_2, AI_PARAM_3 | Não |
| 13 | `AREA_AI_BLUE` | AREA_ID, AI_PARAM_1, AI_PARAM_2, AI_PARAM_3 | Não |
| 14 | `SUB_GOAL_WON` | SUB_GOAL_NUMBER | Sim |
| 15 | `SUB_GOAL_LOST` | SUB_GOAL_NUMBER | Sim |
| 16 | `CHANGE_GTEAM_ACTION` | GROUP, TEAM | Sim |
| 17 | `CHANGE_GROUP_ACTION` | GROUP, GROUP | Sim |
| 18 | `GROUP_TELEPORT_ACTION` | GROUP, TELEPORT_TARGET_NUM | Sim |
| 19 | `REDIRECT_SINGLE_TO` | SINGLE, WAYPOINT_LIST, WAYPOINT_NUMBER | Sim |
| 20 | `KILL_SINGLE` | SINGLE | Sim |
| 21 | `CHANGE_SINGLE_AI` | SINGLE, AI_PARAM_1, AI_PARAM_2, AI_PARAM_3 | Sim |
| 22 | `VAPORIZE_SINGLE` | SINGLE | Sim |
| 23 | `SINGLE_VELOCITY` | SINGLE, VELOCITY_KPH | Não |
| 24 | `CHANGE_STEAM_ACTION` | SINGLE, TEAM | Sim |
| 25 | `SINGLE_CHANGE_GROUP` | SINGLE, GROUP | Sim |
| 26 | `SINGLE_TELEPORT_ACTION` | SINGLE, TELEPORT_TARGET_NUM | Sim |
| 27 | `PARTICLE_EFFECT_ACTION` | TELEPORT_TARGET_NUM | Sim |
| 28 | `UNKNOWN` | - | Não |
| 29 | `UNKNOWN` | - | Não |
| 30 | `GROUP_OPEN_DOOR_ACTION` | GROUP | Não |
| 31 | `GROUP_CLOSE_DOOR_ACTION` | GROUP | Não |
| 32 | `GROUP_RESET_HASVISITED` | GROUP | Sim |
| 33 | `SINGLE_RESET_HASVISITED` | SINGLE | Sim |
| 34 | `RESET_EVENT` | EVENT_NUMBER_STRING | Sim |
| 35 | `SHOW_WIN_SUBGOAL` | SUB_GOAL_NUMBER | Sim |
| 36 | `SHOW_LOSE_SUBGOAL` | SUB_GOAL_NUMBER | Sim |
| 37 | `ATTACH_TO_EMPLACED` | SINGLE | Sim |
| 38 | `SET_LIGHT_STATE` | VARIABLE_NUMBER_STRING, VARIABLE_STATUS | Sim |
| 39 | `TEAMMATES` | TEAMMATES, CONTEXT_TARGET | Sim |
| 40 | `SHOW_WAYPOINTS` | SHOW_WAYPOINTS | Sim |
| 41 | `EXECUTE_WAC` | - | Não |
| 42 | `SSN_TARGET_SSN_PRI` | PERSON, SINGLE | Sim |
| 43 | `SSN_TARGET_SSN_EXC` | PERSON, SINGLE | Sim |
| 44 | `SSN_TARGET_GROUP_PRI` | PERSON, GROUP | Sim |
| 45 | `SSN_TARGET_GROUP_EXC` | PERSON, GROUP | Sim |
| 46 | `GROUP_TARGET_SSN_PRI` | GROUP, SINGLE | Sim |
| 47 | `GROUP_TARGET_SSN_EXC` | GROUP, SINGLE | Sim |
| 48 | `GROUP_TARGET_GROUP_PRI` | GROUP, GROUP | Sim |
| 49 | `GROUP_TARGET_GROUP_EXC` | GROUP, GROUP | Sim |

Os tipos ocultos na lista desta versão são: `OUTPUT_TEXT`, `PLAY_WAVLIST`, `GREEN_WIN`, `GROUP_VELOCITY`, `AREA_AI_RED`, `AREA_AI_BLUE`, `SINGLE_VELOCITY`, `UNKNOWN 28`, `UNKNOWN 29`, `GROUP_OPEN_DOOR_ACTION`, `GROUP_CLOSE_DOOR_ACTION` e `EXECUTE_WAC`. Eles continuam existindo no código de tradução do executável. O mapa enviado contém 35 Actions do tipo 7 `PLAY_WAVLIST`, provando que seu editor precisa aceitar tipos que a UI atual não deixa criar.

## 5. O sétimo inteiro da Action: subtype / AI Action

O significado depende do `action_type`.

### MISVAR_CHANGE, Action Type 5

| Subtype | Operação | Vars |
|---:|---|---|
| 0 | `NULL` | - |
| 1 | `SET_VARIABLE` | MISSION_VARIABLE, VALUE |
| 2 | `ADD_VARIABLE` | MISSION_VARIABLE, VALUE |
| 3 | `SUB_VARIABLE` | MISSION_VARIABLE, VALUE |
| 4 | `INCREMENT_VARIABLE` | MISSION_VARIABLE |
| 5 | `DECREMENT_VARIABLE` | MISSION_VARIABLE |

### TEAMMATES, Action Type 39

| Subtype | Ação | Disponível nesta UI? | Vars |
|---:|---|---|---|
| 0 | `NULL` | Sim | - |
| 1 | `TEAMMATE_CALL_MEDIC` | Não | TEAMMATES, TELEPORT_TARGET_NUM |
| 2 | `TEAMMATE_CALL_EVAC_TT` | Sim | TEAMMATES, TELEPORT_TARGET_NUM |
| 3 | `TEAMMATE_CALL_EVAC_AT` | Sim | TEAMMATES, AREA |

### CHANGE_GROUP_AI / CHANGE_SINGLE_AI

Para Action Types 3 e 21, o sétimo inteiro é o AI Action. A lista que esta UI deixa selecionar é:

`0, 2, 5, 22, 6, 8, 15, 16, 17, 21, 26, 27, 28, 29, 30, 44, 32, 33, 41, 42, 43, 31, 45, 46`.

| ID | Nome | Parâmetros adicionais conhecidos | Selecionável nesta UI? |
|---:|---|---|---|
| 0 | `NULL` | - | Sim |
| 1 | `UNKNOWN` | - | Não |
| 2 | `GUARDER` | GUARD_BIT | Sim |
| 3 | `UNKNOWN` | - | Não |
| 4 | `UNKNOWN` | - | Não |
| 5 | `RED_ALERT` | - | Sim |
| 6 | `GREEN_ALERT` | - | Sim |
| 7 | `UNKNOWN` | - | Não |
| 8 | `ACCURACY` | ACCURACY_100 | Sim |
| 9 | `UNKNOWN` | - | Não |
| 10 | `UNKNOWN` | - | Não |
| 11 | `UNKNOWN` | - | Não |
| 12 | `UNKNOWN` | - | Não |
| 13 | `UNKNOWN` | - | Não |
| 14 | `UNKNOWN` | - | Não |
| 15 | `BLIND` | BLIND_BIT | Sim |
| 16 | `BERSERK` | BERSERK_BIT | Sim |
| 17 | `CLIMBER` | CLIMBER_BIT | Sim |
| 18 | `UNKNOWN` | - | Não |
| 19 | `UNKNOWN` | - | Não |
| 20 | `UNKNOWN` | - | Não |
| 21 | `COWARD` | COWARD_BIT | Sim |
| 22 | `YELLOW_ALERT` | - | Sim |
| 23 | `UNKNOWN` | - | Não |
| 24 | `UNKNOWN` | - | Não |
| 25 | `UNKNOWN` | - | Não |
| 26 | `DRIVESKILL` | SKILL | Sim |
| 27 | `AIMSKILL` | SKILL | Sim |
| 28 | `AISETSTATE` | AISTATE | Sim |
| 29 | `COMBATSPEED` | SPEEDKMH | Sim |
| 30 | `PATROLSPEED` | SPEEDKMH | Sim |
| 31 | `FIND_AND_USE` | TARGETSSN | Sim |
| 32 | `AIUSEWPZ` | - | Sim |
| 33 | `AICLEARWPZ` | - | Sim |
| 34 | `PLAYPARTANIM` | ANIMNUM, ANIMPLAYTYPE, ANIMTIME | Não |
| 35 | `UNKNOWN` | - | Não |
| 36 | `UNKNOWN` | - | Não |
| 37 | `UNKNOWN` | HUDITEM, TICKS | Não |
| 38 | `UNKNOWN` | - | Não |
| 39 | `UNKNOWN` | TMATESTATUS | Não |
| 40 | `AINODEPATH` | AINODEPATH_BIT | Não |
| 41 | `ATTACKDISTANCE` | ATTACKDISTANCE_VALUE | Sim |
| 42 | `ENGAGEDISTANCE` | ENGAGEDISTANCE_MINVALUE, ENGAGEDISTANCE_MAXVALUE | Sim |
| 43 | `INDESTRUCTABLE` | INDESTRUCTABLE_BIT | Sim |
| 44 | `TARGETSSN` | TARGETSSN | Sim |
| 45 | `STARTFIRING` | STARTFIRING_BIT | Sim |
| 46 | `FIRING_ANGLE` | FIRING_ANGLE | Sim |

Valores marcados UNKNOWN devem continuar como números brutos. Não substitua por 0.

## 6. Valores adicionais que a UI não mostra claramente

* `SHOW_WIN_SUBGOAL` e `SHOW_LOSE_SUBGOAL` usam Var1 como número do subgoal. O mapa e o pretty-printer do MED também revelam Var2 como estado: `0` produz HIDE e `1` produz SHOW, embora essa segunda Var não seja apresentada normalmente pela função de labels da Action dialog.
* `SHOW_WAYPOINTS` usa Var1 como estado. O mapa usa 0 no Event de inicialização e 1 posteriormente, coerente com HIDE/SHOW.
* Campos não usados frequentemente aparecem como `-1`, mas não assuma que todo campo não exibido deve virar `-1`. Preserve o valor original.

## 7. O que cada botão da janela Events faz no .mis

### Editar Name / Delay / Reset / Pre / Post

Altera o Event selecionado. `attrib` mantém bits desconhecidos e muda somente `0x01`, `0x02`, `0x04`. `reset_value` e `delay_value` são valores inteiros. Na gravação do arquivo, valores zero são omitidos.

### Criar um Event

A janela não tem um botão Add tradicional. A lista trabalha com uma linha de novo Event no final enquanto a contagem é menor que 256. Ao criar, o MED adiciona um Event zerado no final e aumenta `num_events`.

### Delete Event

O bloco é removido e todos os Events seguintes descem um índice. Em seguida o MED corrige referências:

1. Trigger Type EVENT: se `var1 == índice apagado`, a linha inteira do Trigger é removida. Se `var1 > índice apagado`, `var1--`.
2. Action `RESET_EVENT`: se `var1 >= índice apagado`, `var1--`. Importante: uma Action que apontava exatamente para o Event apagado não é removida. Ela passa numericamente para o índice anterior após o decremento. Se o índice apagado era 0, pode virar -1.
3. Waypoints: se `wp_adv_trigger > índice apagado`, decrementa 1. Se `wp_adv_trigger == índice apagado`, o MED mostra aviso e define `wp_adv_trigger = -1`.
4. Se a contagem chegasse a zero, o MED cria novamente um Event padrão, mantendo pelo menos um Event.

### Copy Event

Copia o bloco inteiro de memória do Event, incluindo nome, attrib, reset/delay, todos os Triggers/Actions e campos reservados. Não altera o `.mis` até Paste.

### Paste Event

Insere o Event copiado antes do Event selecionado e desloca os seguintes. Depois corrige todas as referências com valor `>= índice de inserção`:

* Trigger Type EVENT: `var1++`
* Action RESET_EVENT: `var1++`
* Waypoint `wp_adv_trigger`: `++`

Isso mantém as referências apontando para o mesmo Event lógico depois da inserção.

### Up / Down

Troca os blocos completos dos dois Events envolvidos. Depois troca também referências iguais aos dois índices em:

* Triggers Type EVENT
* Actions RESET_EVENT
* Waypoints `wp_adv_trigger`

Portanto, mover um Event não pode ser implementado apenas reordenando texto no arquivo.

### Print Event / Print All Events

Somente geram representação/relatório. Não alteram o `.mis`.

## 8. Botões de Trigger e Action

### Novo Trigger / Nova Action

A lista usa uma linha vazia adicional quando há espaço. Abrir essa linha e confirmar cria um registro. Limite observado: 20 Triggers e 20 Actions por Event.

### Delete

Remove a linha selecionada, desloca as seguintes e reduz a contagem.

### Copy

Copia os 8 inteiros completos, incluindo valores reservados/ocultos.

### Paste

Insere a linha copiada antes da seleção, deslocando as demais. Não é Replace. Se a seleção for a linha vazia do final, funciona como append. Limite: 20.

### Editar uma linha

A Action dialog altera Action Type, Vars e o campo de subtype/AI Action que no arquivo é o sétimo inteiro. O primeiro e o oitavo inteiros são preservados. O Trigger dialog altera flags/type/condition/Vars e preserva o campo reservado final. Para compatibilidade, seu editor deve manter uma cópia raw dos 8 inteiros e modificar somente os campos efetivamente editados.

## 9. Referência externa a Events que não fica dentro de `begin event`

Waypoints podem conter:

```text
wp_adv_trigger N
```

O executável identifica esses itens como `type_id 6005` (`0x1775`, Waypoint). Inserir, mover ou apagar Events atualiza `wp_adv_trigger`. No mapa enviado existem 9 referências: 15, 41, 55, 68, 77, 110, 92, 114 e 118.

Esse é um dos maiores riscos de um editor novo: renumerar Events sem atualizar Waypoints altera a missão silenciosamente.

## 10. Dados concretos do mapa enviado

* `num_events = 158`
* 375 linhas de Trigger
* 483 linhas de Action
* Maior Event no mapa em quantidade de Triggers: Event 40 com 8
* Maior Event no mapa em quantidade de Actions: Event 114 com 20
* `attrib` observado: 0, 1, 2 e 48
* 3 Triggers usam o tipo oculto `PLAYER`
* 35 Actions usam o tipo oculto `PLAY_WAVLIST`
* 9 Waypoints usam `wp_adv_trigger`

## 11. Exemplo real completamente decodificável

Trecho do Event 15:

```text
begin event 15
  name "Event 15"
  delay_value 5
  begin triggers 1
    2 2 10 10000 10 -1 -1 0
  end triggers
  begin actions 2
    0 14 1 -1 -1 -1 0 0
    0 35 2 1 -1 -1 0 0
  end actions
end event
```

Trigger: flags 2 = True + OR; Type 2 = SINGLE; Condition 10 = AREA_TRIGGER; Var1 = Single 10000; Var2 = Area 10.

Action 1: Type 14 = SUB_GOAL_WON; Var1 = Sub Goal 1.

Action 2: Type 35 = SHOW_WIN_SUBGOAL; Var1 = Sub Goal 2; Var2 = 1, que corresponde a SHOW no pretty-printer.

## 12. Exemplo de recurso oculto usado pelo mapa

Event 144 contém:

```text
0 7 36 1 -1 -1 -1 0
```

como Trigger. Isso é Type 7 PLAYER, Condition 36 PLAYER_AWOL_TEST, TIME_SEC = 1. O Type PLAYER não aparece no dropdown desta versão do MED.

O mesmo Event contém:

```text
0 7 29 0 -1 -1 0 0
```

como Action. Isso é Type 7 PLAY_WAVLIST, WAV_LIST_NUMBER = 29, PLAY_AFTER_END = 0. PLAY_WAVLIST também não aparece no dropdown Action Type desta versão.

## 13. Modelo de dados recomendado para o novo editor

Use uma camada raw e uma camada decoded. Exemplo conceitual:

```text
Event
  fileIndex
  stableInternalId
  name
  attribRaw
  resetValue
  delayValue
  triggers[]
  actions[]

Trigger
  raw[8]
  flags       = raw[0]
  type        = raw[1]
  condition   = raw[2]
  vars        = raw[3..6]
  reserved7   = raw[7]

Action
  raw[8]
  reserved0   = raw[0]
  type        = raw[1]
  vars        = raw[2..5]
  subtype     = raw[6]
  reserved7   = raw[7]
```

Nunca use o índice do Event como identidade interna permanente no seu app. Dê a cada Event um ID interno estável. O índice só deve ser calculado na hora de exibir/serializar. Assim, mover Events fica muito mais seguro.

## 14. Regras mínimas de compatibilidade

1. Preserve `attrib` desconhecido.
2. Preserve todos os 8 inteiros raw de cada Trigger/Action.
3. Não converta automaticamente campos ocultos para `-1` ou 0.
4. Suporte tipos/subtipos conhecidos pelo EXE mesmo quando não aparecem nos dropdowns atuais.
5. Mostre valores desconhecidos como `Unknown (N)`, mantendo N.
6. Ao inserir/deletar/mover Event, atualize EVENT Trigger, RESET_EVENT e `wp_adv_trigger`.
7. Lembre que RESET_EVENT usa `var1`, que é o terceiro inteiro da linha de Action no arquivo.
8. Lembre que EVENT Trigger usa `var1`, que é o quarto inteiro da linha de Trigger no arquivo.
9. Serializar Action com subtype no sétimo inteiro, nunca no terceiro.
10. Respeite pelo menos os limites do MED: 256 Events, 20 Triggers/Event, 20 Actions/Event.

## 15. O que ainda deve ser tratado como desconhecido

Há IDs chamados `UNKNOWN` no próprio código e alguns bits/valores que a janela Events não dá significado. Eles não precisam impedir seu editor de funcionar. A política correta é round-trip lossless: ler, exibir como valor raw quando necessário e gravar de volta sem alteração.

O arquivo `dfbhd_events_schema.json` acompanha este guia com as tabelas em formato mais fácil de consumir pelo código do novo editor.
