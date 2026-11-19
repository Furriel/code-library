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
- RESUMO.txt, operacao.log e ZIP para revisao

## Aplicar um plano revisado

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
