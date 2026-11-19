# Auditoria e limpeza de D:\GitHub\Furriel

Utilitario versionado no GitHub. **Auditoria e limpeza sao etapas separadas.** O caminho local esperado e:
D:\GitHub\Furriel\code-library\ferramentas\limpeza-furriel

## Instalar ou atualizar + auditar

Execute no **PowerShell do Windows** (pode colar o bloco inteiro):

    $repo = 'D:\GitHub\Furriel\code-library'
    if (Test-Path (Join-Path $repo '.git')) {
        git -C $repo fetch origin main
        if ($LASTEXITCODE -ne 0) { throw 'Falha ao buscar origin/main.' }
        git -C $repo switch main
        if ($LASTEXITCODE -ne 0) { throw 'Nao foi possivel mudar para main; preserve alteracoes locais.' }
        git -C $repo pull --ff-only origin main
    } elseif (-not (Test-Path $repo)) {
        git clone --branch main https://github.com/Furriel/code-library.git $repo
    } else {
        throw "Pasta existente sem Git: $repo"
    }
    if ($LASTEXITCODE -ne 0) { throw 'Falha no clone/pull; nada sera executado.' }
    $script = Join-Path $repo 'ferramentas\limpeza-furriel\Executar-Auditoria.ps1'
    if (-not (Test-Path -LiteralPath $script)) { throw "Script nao encontrado: $script" }
    & $script
A atualizacao explicita da main evita executar a branch errada. Se houver mudancas locais que impeçam o switch, o Git bloqueia a operacao sem descartá-las. A auditoria faz git fetch --prune
em cada repositorio encontrado, sem alterar arquivos de trabalho.

O pacote de retorno fica em D:\GitHub\_Auditorias_Furriel\DATA-HORA-PID.zip.

## Saidas

- repositorios.csv — comparacao remota/local, alteracoes locais e branch padrao
- branches.csv — branches locais/remotas, ancestrais da base, PRs e worktrees
- arquivos.csv — caches, logs, relatorios, builds e dependencias
- plano.csv — acoes propostas, com coluna Autorizar (SIM/NAO)
- docker-inventario.txt — containers, Compose, imagens, volumes, espaco
- duplicados.csv — copias locais que apontam para o mesmo repositorio GitHub
- pastas-revisao.csv — validacao, releases, snapshots e logs na raiz (apenas inventario)
- RESUMO.txt, operacao.log e ZIP para revisao

## Preparar plano revisado (segunda auditoria de 19/11/2026)

A auditoria revisada encontrou **39 diretorios Git**, **871 branches**, **160 entradas
no plano** e **47 pastas de validacao/logs**. Cerca de 6,92 GB estavam em
dependencias, dos quais 5,17 GB no GroveAI_SkinCancer\.venv-train.

O script Preparar-Plano.ps1 cria **plano-revisado.csv** na propria pasta da
auditoria, sem excluir nada. Exemplo de selecao conservadora:

    $base = 'D:\GitHub\Furriel\code-library\ferramentas\limpeza-furriel'
    & "$base\Preparar-Plano.ps1" -PlanoOrigem 'D:\GitHub\_Auditorias_Furriel\20261119-123728-20336\plano.csv' -IncluirBranchesLocais -IncluirNodeModules -IncluirBuilds

Esta selecao inclui caches descartaveis, branches locais incorporadas,
node_modules com arquivo lock na pasta imediatamente superior e builds
de repositorios sem alteracoes locais. Nao inclui as 73 branches remotas,
logs de homologacao, copias do Silico Board nem os 5,17 GB do ambiente de
treino GroveAI.

**Excecao permanente:** por determinacao do usuario, o repositorio
GroveAI_SkinCancer esta protegido integralmente (arquivos, caches, .venv-train,
branch master, branches remotas e imagens Docker). A opcao
-IncluirAmbienteTreino NAO permite excluir o GroveAI, nem usando planos antigos.

Revise o plano-revisado.csv antes de aplicar:

    & "$base\Limpar-Furriel.ps1" -Aplicar -Plano 'D:\GitHub\_Auditorias_Furriel\20261119-123728-20336\plano-revisado.csv'

Aplicar exige confirmacao digitada. O script revalida SHAs, PRs, worktrees,
Git e caminhos antes das exclusoes. O `-Aplicar` nao foi executado
remotamente. Em caso de desenvolvimento ativo, considere reinstalar
dependencias antes de retomar o trabalho (npm ci, pnpm install etc.).

## Aplicar um plano revisado

**Importante (19/11/2026):** a auditoria inicial identificou 791 caches
dentro do ambiente virtual GroveAI_SkinCancer\.venv-train. A versao atual
agora detecta .venv-* como dependencia unica e BLOQUEIA a exclusao de
pastas internas, mesmo usando planos antigos. Execute outra auditoria
apos atualizar o Git antes de aplicar uma limpeza.

**Nenhuma exclusao ocorre na primeira auditoria.** Revise o CSV e autorize
somente os itens que pretende remover. No PowerShell:

    $dir = 'D:\GitHub\Furriel\code-library\ferramentas\limpeza-furriel'
    & "$dir\Limpar-Furriel.ps1" -Aplicar -Plano 'D:\GitHub\_Auditorias_Furriel\DATA-HORA-PID\plano.csv'

Confirme digitando APLICAR LIMPEZA. Antes de excluir, revalida caminhos,
arquivos rastreados, data de modificacao, commits, branch padrao, PRs e
worktrees. Branches locais exigem ancestralidade real de origin/default;
squash e rebase nao sao considerados integrados automaticamente.

As branches remotas vem com Autorizar=NAO, exigem -ExcluirRemotas e uma
confirmacao individual EXCLUIR REMOTA. A exclusao utiliza force-with-lease.

## Docker

O inventario e somente leitura. **Docker prune global esta BLOQUEADO**
no script, pois imagens e caches Docker do GroveAI nao podem ser
identificados com seguranca. Nenhuma imagem, container, volume ou cache
Docker e removido nesta etapa.

## Etapa 2 — Branches remotas (novembro de 2026)

O ultimo relatorio de limpeza confirmou 55 exclusoes locais e 4 referencias
ja ausentes. Existem 73 registros de branches remotas, mas sete entradas sao
duplicadas entre IFAEE e IFAEE-VALIDACAO (66 branches distintas).

A nova ferramenta Limpar-Remotas.ps1 tem **modo de auditoria por padrao**.
Ela exige Git e GitHub CLI autenticados. A analise:

1. Deduplica por repositorio GitHub e nome de branch.
2. Bloqueia GroveAI, Silico Board, artigos, review e energy_analyzer.
3. Preserva backups, releases, branches de staging/rollback e tags de versao.
4. Exige commit com ao menos 14 dias, incorporado por ancestralidade.
5. Verifica branch padrao, SHA remota, PRs abertos e worktrees.
6. Gera plano-remotas.csv e um ZIP sem excluir nada.

No PowerShell:

    $repo = 'D:\GitHub\Furriel\code-library'
    git -C $repo switch main
    if ($LASTEXITCODE -ne 0) { throw 'Falha no switch para main' }
    git -C $repo pull --ff-only origin main
    if ($LASTEXITCODE -ne 0) { throw 'Falha no pull' }
    $script = "$repo\ferramentas\limpeza-furriel\Limpar-Remotas.ps1"
    & $script -PlanoAuditoria 'D:\GitHub\_Auditorias_Furriel\20261119-123728-20336\plano.csv'

O plano e gerado em D:\GitHub\_Auditorias_Furriel\REMOTAS-...,
com as branches ELEGIVEL/Autorizar=SIM. **Revise o arquivo antes de aplicar.**

    & $script -Aplicar -PlanoRemotas 'D:\GitHub\_Auditorias_Furriel\REMOTAS-...\plano-remotas.csv'

A exclusao requer digitar EXCLUIR REMOTAS. A validacao e refeita na hora
e o push utiliza --force-with-lease. Na falta de GitHub CLI, acesso aos PRs,
ancestralidade, branch padrao ou SHA igual, a exclusao e bloqueada.
A operacao nao afeta worktrees nem arquivos de teste/validacao.

## Auditoria das copias de validacao (sem exclusao)

O script Analisar-Copias.ps1 reutiliza duplicados.csv e pastas-revisao.csv
para conferir worktrees, clones, branch atual, HEAD, alteracoes nao enviadas,
divergencias com origin/HEAD e existencia das pastas.

    $base = 'D:\GitHub\Furriel\code-library\ferramentas\limpeza-furriel'
    & "$base\Analisar-Copias.ps1" -AuditoriaAnterior 'D:\GitHub\_Auditorias_Furriel\20261119-123728-20336'

O resultado sera salvo em D:\GitHub\_Auditorias_Furriel\COPIAS-...zip.
Nao ha comando de exclusao de pastas neste script. Relatorios Silico B03/B04
e worktrees sao classificados como protegidos.

## Duplicatas e evidencias

Diretorios de clones e snapshots (especialmente Silico Board, EngQuest e SAPE)
sao inventariados mas nao entram no plano de exclusao automatica.
Avalie PRs, worktrees, diferencas locais, arquivos nao rastreados, backups,
historico de homologacao e dados externos antes de qualquer remocao.
Em instalacoes com muitos repositórios, -MedirTamanhos mede o espaco
aproximado de pastas detectadas (e torna a auditoria mais demorada).

## Regras de seguranca

- Nao apaga repositorios, pastas .git, branches protegidas, alteracoes locais,
  commits nao incorporados ou arquivos rastreados.
- Nao usa git reset, git clean -fdx, push --force de historico, nem docker volume prune.
- Se fetch falhar ou a branch padrao nao for verificavel, a branch nao pode
  virar candidata de exclusao.
- Caches ignorados pelo Git e antigos podem vir marcados SIM; outros arquivos
  e dependencias exigem revisao manual.
- Os arquivos excluidos nao sao movidos para a Lixeira.
- Requer Git para Windows; GitHub CLI (gh) autenticado e recomendado para
  verificar branch padrao e PRs. Docker Desktop e opcional.
- Requer PowerShell 5.1 ou superior; primeira execucao ainda nao foi testada
  no computador do usuario.

## Recuperacao de uma branch local removida

O CSV branches.csv guarda seu SHA anterior. Para reconstruir:

    git -C 'D:\GitHub\Furriel\NOME_REPO' branch NOME_BRANCH SHA_ANTERIOR

Nao presume que relatórios de execucao antigos sao descartaveis: ficam
marcados NAO ate revisao.
