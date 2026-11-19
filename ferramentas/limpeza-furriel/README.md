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

Para incluir **explicitamente** o ambiente de treino, acrescente
-IncluirAmbienteTreino. A existencia de requirements.txt e pyproject.toml
nao garante que seja possivel reproduzir bibliotecas e modelos exatamente
como no ambiente anterior; preserve o ambiente se tiver duvidas.

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

O inventario e somente leitura. **Docker prune afeta a maquina inteira.**
Se desejar limpar manualmente, acrescente -LimparDocker -DiasDocker 30
na execucao com -Aplicar. Exige LIMPAR DOCKER GLOBAL. Elimina apenas
containers parados, imagens dangling e cache de build antigos; nao apaga
volumes, containers ativos, nem executa docker compose down.

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
