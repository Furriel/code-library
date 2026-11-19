#Requires -Version 5.1
<#
.SYNOPSIS
Audita D:\GitHub\Furriel e prepara limpeza local verificavel.
.DESCRIPTION
A auditoria nao apaga arquivos nem branches. A aplicacao exige plano.csv,
confirmacao e revalidacao de SHA, arquivos rastreados, PRs e worktrees.
Nunca executa git reset, git clean, docker volume prune ou docker compose down.
#>
[CmdletBinding()]
param(
    [string]$Raiz = 'D:\GitHub\Furriel',
    [string]$Saida = 'D:\GitHub\_Auditorias_Furriel',
    [int]$DiasMinimos = 7,
    [int]$ProfundidadeMaxima = 7,
    [switch]$IncluirRelatorios,
    [switch]$IncluirBuilds,
    [switch]$IncluirDependencias,
    [switch]$IncluirDocker,
    [switch]$MedirTamanhos,
    [switch]$SemFetch,
    [switch]$Aplicar,
    [string]$Plano,
    [switch]$ExcluirRemotas,
    [switch]$LimparDocker,
    [int]$DiasDocker = 30
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$env:GIT_TERMINAL_PROMPT = '0'
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git nao localizado no PATH.' }
if (-not (Test-Path -LiteralPath $Raiz -PathType Container)) { throw "Raiz nao encontrada: $Raiz" }
if ($DiasMinimos -lt 0 -or $ProfundidadeMaxima -lt 1 -or $DiasDocker -lt 1) { throw 'Parametros de idade/profundidade invalidos.' }
$Raiz = (Resolve-Path -LiteralPath $Raiz).ProviderPath.TrimEnd('\','/')
New-Item -ItemType Directory -Path $Saida -Force | Out-Null
$Saida = (Resolve-Path -LiteralPath $Saida).ProviderPath.TrimEnd('\','/')
if ($Saida -eq $Raiz -or $Saida.StartsWith($Raiz + '\',[StringComparison]::OrdinalIgnoreCase)) {
    throw 'A saida de auditoria deve ficar FORA da raiz auditada.'
}
$ExecDir = Join-Path $Saida ((Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $PID)
New-Item -ItemType Directory -Path $ExecDir -Force | Out-Null
$Log = Join-Path $ExecDir 'operacao.log'
$Protegidas = @('main','master','dev','develop','development','staging','production','prod','release/*','release-*','stable/*','backup/*','backup-*','bkp-*','snapshot/*','rescue/*','rollback/*','homolog/*','integracao/*')
$Caches = @('playwright-report','test-results','coverage','htmlcov','.pytest_cache','.mypy_cache','.ruff_cache','.turbo','.parcel-cache','__pycache__','.nyc_output','.tox')
$Builds = @('dist','build','out','target','.next','.nuxt','.svelte-kit')
$Dependencias = @('node_modules','.venv','venv')
$Pular = @('.git','.hg','.svn','node_modules','.venv','venv','vendor','target','dist','build','out','.next','.nuxt','__pycache__','.pytest_cache','.cache','.turbo','coverage','playwright-report','test-results')
$Repos = [Collections.Generic.List[object]]::new()
$Branches = [Collections.Generic.List[object]]::new()
$Arquivos = [Collections.Generic.List[object]]::new()
$Acoes = [Collections.Generic.List[object]]::new()
$Execucoes = [Collections.Generic.List[object]]::new()

function Escrever([string]$Mensagem) {
    $linha = ('[{0}] {1}' -f (Get-Date -Format 's'),$Mensagem)
    Write-Host $linha
    Add-Content -LiteralPath $script:Log -Value $linha -Encoding UTF8
}
function Executar([string]$Programa,[string[]]$Argumentos) {
    $anterior = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $executavel = if ($Programa -eq 'git') { (Get-Command -Name 'git.exe' -CommandType Application -ErrorAction Stop).Source } else { $Programa }
        $linhas = @(& $executavel @Argumentos 2>$null)
        $codigo = $LASTEXITCODE
    } finally { $ErrorActionPreference = $anterior }
    return [pscustomobject]@{ OK=($codigo -eq 0); Texto=($linhas -join [Environment]::NewLine).Trim(); Linhas=@($linhas) }
}
function Git([string]$Repo,[string[]]$Argumentos) {
    return Executar 'git' (@('-C',$Repo) + $Argumentos)
}
function Abaixo([string]$Caminho,[string]$Base) {
    try { $absoluto=[IO.Path]::GetFullPath($Caminho).TrimEnd('\','/') }
    catch { return $false }
    return $absoluto.StartsWith($Base + '\',[StringComparison]::OrdinalIgnoreCase)
}
function RepositorioProtegido([string]$Repo) {
    if ($Repo -match '(?i)(?:^|[\\/])GroveAI_SkinCancer(?:[\\/]|$)') { return $true }
    $url=Git $Repo @('remote','get-url','origin')
    if ($url.OK -and $url.Texto -match '(?i)github\.com[:/]Furriel/GroveAI_SkinCancer(?:\.git)?$') { return $true }
    return $false
}
function Protegida([string]$Branch,[string]$Padrao) {
    if ($Branch -eq $Padrao) { return $true }
    foreach($p in $script:Protegidas) { if ($Branch -like $p) { return $true } }
    return $false
}
function Ancestral([string]$Repo,[string]$Sha,[string]$Base) {
    if (-not $Sha -or -not $Base) { return $false }
    return (Git $Repo @('merge-base','--is-ancestor',$Sha,$Base)).OK
}
function Slug([string]$Repo) {
    $url=(Git $Repo @('remote','get-url','origin')).Texto
    if ($url -match '(?i)github\.com[:/]([^/]+)/([^/]+?)(?:\.git)?$') {
        return ($Matches[1] + '/' + $Matches[2])
    }
    return ''
}
function Padrao([string]$Repo) {
    $s=Slug $Repo
    if ($s -and (Get-Command gh -ErrorAction SilentlyContinue)) {
        $v=Executar 'gh' @('repo','view',$s,'--json','defaultBranchRef','--jq','.defaultBranchRef.name')
        if ($v.OK -and $v.Texto) { return $v.Texto }
    }
    $r=Git $Repo @('symbolic-ref','--quiet','--short','refs/remotes/origin/HEAD')
    if ($r.OK -and $r.Texto.StartsWith('origin/')) { return $r.Texto.Substring(7) }
    return ''
}
function PRs([string]$Repo) {
    $s=Slug $Repo
    if (-not $s -or -not (Get-Command gh -ErrorAction SilentlyContinue)) {
        return [pscustomobject]@{ Verificado=$false; Nomes=@() }
    }
    $r=Executar 'gh' @('pr','list','-R',$s,'--state','open','--json','headRefName','--limit','1000')
    if (-not $r.OK) { return [pscustomobject]@{ Verificado=$false; Nomes=@() } }
    try {
        $dados=@($r.Texto | ConvertFrom-Json)
        return [pscustomobject]@{ Verificado=$true; Nomes=@($dados | ForEach-Object { $_.headRefName } | Where-Object { $_ }) }
    } catch { return [pscustomobject]@{ Verificado=$false; Nomes=@() } }
}
function Worktrees([string]$Repo) {
    $r=Git $Repo @('worktree','list','--porcelain')
    if (-not $r.OK) { return @() }
    return @($r.Linhas | Where-Object { $_ -like 'branch refs/heads/*' } | ForEach-Object { $_.Substring(18) })
}
function DependenciaNome([string]$Nome) {
    return ($Nome -in $script:Dependencias -or $Nome -match '^(?:\.venv(?:[-_].+)?|venv(?:[-_].+)?)$')
}
function DentroDependencia([string]$Repo,[string]$Caminho) {
    if (-not (Abaixo $Caminho $Repo)) { return $true }
    $rel=$Caminho.Substring($Repo.Length).TrimStart('\','/')
    $partes=@($rel -split '[\\/]')
    if ($partes.Count -lt 2) { return $false }
    for($i=0; $i -lt ($partes.Count - 1); $i++) {
        if ((DependenciaNome $partes[$i]) -or $partes[$i] -eq 'site-packages') { return $true }
    }
    return $false
}
function Repositorios {
    $achados=[Collections.Generic.List[string]]::new()
    $fila=[Collections.Generic.Queue[object]]::new()
    $fila.Enqueue([pscustomobject]@{ Caminho=$Raiz; Nivel=0 })
    while ($fila.Count) {
        $item=$fila.Dequeue()
        if (Test-Path -LiteralPath (Join-Path $item.Caminho '.git')) { $achados.Add($item.Caminho); continue }
        if ($item.Nivel -ge $ProfundidadeMaxima) { continue }
        foreach($dir in @(Get-ChildItem -LiteralPath $item.Caminho -Directory -Force -ErrorAction SilentlyContinue)) {
            if ($dir.Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
            if ($dir.Name -in $Pular -or (DependenciaNome $dir.Name)) { continue }
            $fila.Enqueue([pscustomobject]@{ Caminho=$dir.FullName; Nivel=($item.Nivel+1) })
        }
    }
    return $achados.ToArray()
}
function Tamanho([string]$Caminho) {
    if (-not $MedirTamanhos) { return '' }
    try {
        $item=Get-Item -LiteralPath $Caminho -Force
        if (-not $item.PSIsContainer) { return [string]$item.Length }
        $total=(Get-ChildItem -LiteralPath $Caminho -File -Recurse -Force -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum
        return [string][int64]$total
    } catch { return '' }
}
function Rastreado([string]$Repo,[string]$Caminho) {
    $rel=$Caminho.Substring($Repo.Length).TrimStart('\','/') -replace '\\','/'
    $r=Git $Repo @('ls-files','--cached','--',$rel)
    if (-not $r.OK) { return $true }
    return -not [string]::IsNullOrWhiteSpace($r.Texto)
}
function Ignorado([string]$Repo,[string]$Caminho) {
    $rel=$Caminho.Substring($Repo.Length).TrimStart('\','/') -replace '\\','/'
    return (Git $Repo @('check-ignore','--quiet','--',$rel)).OK
}
function Adicionar([string]$Tipo,[string]$Repo,[string]$Alvo,[string]$Sha,[string]$Evidencia,[string]$Data,[string]$Bytes,[string]$Autorizacao) {
    $script:Acoes.Add([pscustomobject][ordered]@{
        Tipo=$Tipo; Repositorio=$Repo; Alvo=$Alvo; SHA=$Sha;
        Evidencia=$Evidencia; ModificadoEm=$Data; Bytes=$Bytes; Autorizar=$Autorizacao
    })
}
function RelatorioCandidato([string]$Nome) {
    return ($Nome -match '(?i)\.(?:log|trace|trx|profraw)$' -or
            $Nome -match '(?i)^(?:junit[^\\]*\.xml|pytest-[^\\]*\.xml|relatorio[-_]?execucao[^\\]*\.(?:json|log|txt|html)|report[-_]?execucao[^\\]*\.(?:json|log|html)|SILICO-[^\\]*-(?:PASS|FAIL)\.zip)$')
}
function AvaliarArquivos([string]$Repo,[datetime]$Limite) {
    $fila=[Collections.Generic.Queue[object]]::new()
    $fila.Enqueue([pscustomobject]@{ Caminho=$Repo; Nivel=0 })
    while ($fila.Count) {
        $item=$fila.Dequeue()
        if ($item.Nivel -ge $ProfundidadeMaxima) { continue }
        foreach($dir in @(Get-ChildItem -LiteralPath $item.Caminho -Directory -Force -ErrorAction SilentlyContinue)) {
            if ($dir.Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
            if ($dir.Name -eq '.git' -or (Test-Path -LiteralPath (Join-Path $dir.FullName '.git'))) { continue }
            $tipo=''
            if (DependenciaNome $dir.Name) {
                if (-not $IncluirDependencias) { continue }
                $tipo='Dependencia'
            }
            elseif ($dir.Name -in $Caches) { $tipo='CachePasta' }
            elseif ($IncluirBuilds -and $dir.Name -in $Builds) { $tipo='BuildPasta' }
            if ($tipo) {
                $tracked=Rastreado $Repo $dir.FullName
                $ignorado=Ignorado $Repo $dir.FullName
                $elegivel=(-not $tracked) -and $dir.LastWriteTime -le $Limite
                $bytes=Tamanho $dir.FullName
                $script:Arquivos.Add([pscustomobject]@{ Repositorio=$Repo; Caminho=$dir.FullName; Tipo=$tipo; Elegivel=$elegivel; Ignorado=$ignorado; Rastreado=$tracked; Bytes=$bytes })
                if ($elegivel) {
                    # Caches ignorados podem ser aprovados por padrao. Builds e dependencias requerem revisao.
                    $aprovacao=if ($tipo -eq 'CachePasta' -and $ignorado) { 'SIM' } else { 'NAO' }
                    Adicionar $tipo $Repo $dir.FullName '' 'Pasta reconstruivel; sem arquivos rastreados; verificar valor dos dados' $dir.LastWriteTime.ToString('o') $bytes $aprovacao
                }
                continue
            }
            if ($dir.Name -in $Pular) { continue }
            $fila.Enqueue([pscustomobject]@{ Caminho=$dir.FullName; Nivel=($item.Nivel+1) })
        }
        if (-not $IncluirRelatorios) { continue }
        foreach($f in @(Get-ChildItem -LiteralPath $item.Caminho -File -Force -ErrorAction SilentlyContinue)) {
            if (-not (RelatorioCandidato $f.Name)) { continue }
            $tracked=Rastreado $Repo $f.FullName
            $elegivel=(-not $tracked) -and $f.LastWriteTime -le $Limite
            $script:Arquivos.Add([pscustomobject]@{ Repositorio=$Repo; Caminho=$f.FullName; Tipo='RelatorioTemporario'; Elegivel=$elegivel; Ignorado=(Ignorado $Repo $f.FullName); Rastreado=$tracked; Bytes=$f.Length })
            if ($elegivel) {
                Adicionar 'RelatorioTemporario' $Repo $f.FullName '' 'Arquivo sem rastreamento Git; confirmar que nao e evidencia necessaria' $f.LastWriteTime.ToString('o') ([string]$f.Length) 'NAO'
            }
        }
    }
}
function Csv([object[]]$Dados,[string]$Caminho,[string]$Cabecalho) {
    if ($Dados.Count) { $Dados | Export-Csv -LiteralPath $Caminho -Encoding UTF8 -NoTypeInformation }
    else { Set-Content -LiteralPath $Caminho -Encoding UTF8 -Value $Cabecalho }
}
function Contexto([string]$Repo) {
    $fetch=$false
    if (-not $SemFetch) {
        $fetch=(Git $Repo @('fetch','--prune','origin')).OK
        if ($fetch) { [void](Git $Repo @('remote','set-head','origin','-a')) }
    }
    $padrao=Padrao $Repo
    $base=if ($padrao) { 'refs/remotes/origin/' + $padrao } else { '' }
    $valido=$fetch -and $base -and (Git $Repo @('show-ref','--verify','--quiet',$base)).OK
    $dirty= -not [string]::IsNullOrWhiteSpace((Git $Repo @('status','--porcelain=v1')).Texto)
    return [pscustomobject]@{ Repo=$Repo; Fetch=$fetch; Padrao=$padrao; Base=$base; Valido=[bool]$valido; Dirty=$dirty; PR=(PRs $Repo); Worktrees=@(Worktrees $Repo) }
}
function Auditoria {
    $lista=@(Repositorios)
    Escrever "Repositorios encontrados: $($lista.Count)"
    $limite=(Get-Date).AddDays(-$DiasMinimos)
    foreach($repo in $lista) {
        if (RepositorioProtegido $repo) {
            Escrever "PROTEGIDO: $repo (GroveAI), sem fetch, plano ou exclusoes"
            $script:Repos.Add([pscustomobject]@{ Repositorio=$repo; GitHub='Furriel/GroveAI_SkinCancer'; BranchPadrao=''; FetchOK=$false; ComparacaoConfiavel=$false; AlteracoesLocais=$true; PRsVerificados=$false })
            continue
        }
        Escrever "Auditando $repo"
        $d=Contexto $repo
        $script:Repos.Add([pscustomobject]@{ Repositorio=$repo; GitHub=(Slug $repo); BranchPadrao=$d.Padrao; FetchOK=$d.Fetch; ComparacaoConfiavel=$d.Valido; AlteracoesLocais=$d.Dirty; PRsVerificados=$d.PR.Verificado })
        $refs=Git $repo @('for-each-ref','--format=%(refname:short)|%(objectname)|%(upstream:short)|%(upstream:track)','refs/heads')
        if ($refs.OK) {
            foreach($linha in $refs.Linhas) {
                $p=$linha -split '\|',4
                if ($p.Count -ne 4) { continue }
                $nome=$p[0]; $sha=$p[1]
                $protegido=Protegida $nome $d.Padrao
                $emUso=$nome -in $d.Worktrees
                $prAberto=$d.PR.Verificado -and $nome -in $d.PR.Nomes
                $merged=$d.Valido -and (Ancestral $repo $sha $d.Base)
                $permitido=$merged -and -not $protegido -and -not $emUso -and -not $prAberto -and $d.PR.Verificado -and -not $d.Dirty
                $script:Branches.Add([pscustomobject]@{ Repositorio=$repo; Origem='Local'; Branch=$nome; SHA=$sha; Upstream=$p[2]; UpstreamStatus=$p[3]; Protegida=$protegido; Worktree=$emUso; PRaberto=$prAberto; Incorporada=$merged; Candidata=$permitido })
                if ($permitido) { Adicionar 'BranchLocal' $repo $nome $sha 'Commit incorporado na branch padrao; worktree livre; repositorio limpo' '' '' 'SIM' }
            }
        }
        $refs=Git $repo @('for-each-ref','--format=%(refname:short)|%(objectname)','refs/remotes/origin')
        if ($refs.OK) {
            foreach($linha in $refs.Linhas) {
                $p=$linha -split '\|',2
                if ($p.Count -ne 2 -or $p[0] -eq 'origin/HEAD' -or -not $p[0].StartsWith('origin/')) { continue }
                $nome=$p[0].Substring(7); $sha=$p[1]
                $protegido=Protegida $nome $d.Padrao
                $prAberto=$d.PR.Verificado -and $nome -in $d.PR.Nomes
                $merged=$d.Valido -and (Ancestral $repo $sha $d.Base)
                $permitido=$merged -and -not $protegido -and $d.PR.Verificado -and -not $prAberto -and -not $d.Dirty
                $script:Branches.Add([pscustomobject]@{ Repositorio=$repo; Origem='Remota'; Branch=$nome; SHA=$sha; Upstream=''; UpstreamStatus=''; Protegida=$protegido; Worktree=$false; PRaberto=$prAberto; Incorporada=$merged; Candidata=$permitido })
                if ($permitido) { Adicionar 'BranchRemota' $repo $nome $sha 'Ancestral do branch padrao; sem PR aberto; requer autorizacao individual' '' '' 'NAO' }
            }
        }
        AvaliarArquivos $repo $limite
    }
    $duplicados=@(
        foreach($grupo in @($script:Repos.ToArray() | Where-Object { $_.GitHub } | Group-Object -Property GitHub | Where-Object { $_.Count -gt 1 })) {
            foreach($copia in $grupo.Group) {
                [pscustomobject]@{
                    GitHub=$grupo.Name; Copias=$grupo.Count; Caminho=$copia.Repositorio
                    AlteracoesLocais=$copia.AlteracoesLocais
                    Acao='REVISAR_SEPARADAMENTE_NUNCA_EXCLUIR_AUTOMATICAMENTE'
                }
            }
        }
    )
    Csv $duplicados (Join-Path $ExecDir 'duplicados.csv') 'GitHub,Copias,Caminho,AlteracoesLocais,Acao'
    $pastas=@(
        foreach($p in @(Get-ChildItem -LiteralPath $Raiz -Directory -Force -ErrorAction SilentlyContinue)) {
            if ($p.Name -match '(?i)(validac[aã]o|release|logs|backup|snapshot|b0[0-9])') {
                [pscustomobject]@{
                    Nome=$p.Name; Caminho=$p.FullName
                    ModificadoEm=$p.LastWriteTime.ToString('o')
                    Bytes=(Tamanho $p.FullName)
                    Acao='REVISAR_SEPARADAMENTE_NUNCA_EXCLUIR_AUTOMATICAMENTE'
                }
            }
        }
    )
    Csv $pastas (Join-Path $ExecDir 'pastas-revisao.csv') 'Nome,Caminho,ModificadoEm,Bytes,Acao'
    Csv $script:Repos.ToArray() (Join-Path $ExecDir 'repositorios.csv') 'Repositorio,GitHub,BranchPadrao,FetchOK,ComparacaoConfiavel,AlteracoesLocais,PRsVerificados'
    Csv $script:Branches.ToArray() (Join-Path $ExecDir 'branches.csv') 'Repositorio,Origem,Branch,SHA,Upstream,UpstreamStatus,Protegida,Worktree,PRaberto,Incorporada,Candidata'
    Csv $script:Arquivos.ToArray() (Join-Path $ExecDir 'arquivos.csv') 'Repositorio,Caminho,Tipo,Elegivel,Ignorado,Rastreado,Bytes'
    Csv $script:Acoes.ToArray() (Join-Path $ExecDir 'plano.csv') 'Tipo,Repositorio,Alvo,SHA,Evidencia,ModificadoEm,Bytes,Autorizar'
    @(
        "Raiz: $Raiz",
        "Repositorios: $($script:Repos.Count)",
        "Copias de repositorios para revisar: $($duplicados.Count)",
        "Pastas de validacao/logs para revisar: $($pastas.Count)",
        'Ambientes virtuais sao inventariados como uma dependencia; seu conteudo interno nao e listado.',
        "Branches inventariadas: $($script:Branches.Count)",
        "Itens propostos: $($script:Acoes.Count)",
        'Para aprovar, revise o plano.csv e edite somente Autorizar=SIM/NAO.',
        'Remotas, builds, dependencias e relatorios nunca sao aprovados automaticamente.',
        'Squash/rebase sem ancestralidade nao gera exclusao automatica.',
        'Limpeza: executar Limpar-Furriel.ps1 -Aplicar -Plano CAMINHO_COMPLETO.',
        'Sem exclusao de repositorios, volumes, branches nao incorporadas ou .git.'
    ) | Set-Content -LiteralPath (Join-Path $ExecDir 'RESUMO.txt') -Encoding UTF8
    Escrever "Plano gerado em: $ExecDir"
}
function ValidarBranch([object]$Item) {
    $repo=[string]$Item.Repositorio; $nome=[string]$Item.Alvo; $tipo=[string]$Item.Tipo; $sha=[string]$Item.SHA
    if (RepositorioProtegido $repo) { return 'GroveAI protegido integralmente' }
    if (-not (Abaixo $repo $Raiz) -or -not (Test-Path -LiteralPath (Join-Path $repo '.git'))) { return 'Repositorio invalido' }
    $d=Contexto $repo
    if (-not $d.Valido -or $d.Dirty) { return 'Git nao atualizado ou alteracoes locais' }
    if (Protegida $nome $d.Padrao) { return 'Branch protegida' }
    if ($nome -in $d.Worktrees) { return 'Branch usada em worktree' }
    if (-not $d.PR.Verificado) { return 'Nao foi possivel verificar PRs abertos' }
    if ($nome -in $d.PR.Nomes) { return 'PR aberto' }
    if ($tipo -eq 'BranchRemota' -and -not $d.PR.Verificado) { return 'Verificacao dos PRs indisponivel' }
    $ref=if ($tipo -eq 'BranchLocal') { 'refs/heads/' + $nome } else { 'refs/remotes/origin/' + $nome }
    $atual=Git $repo @('rev-parse','--verify',$ref)
    if (-not $atual.OK -or $atual.Texto -ne $sha) { return 'SHA mudou ou branch ausente' }
    if (-not (Ancestral $repo $sha $d.Base)) { return 'Nao incorporada na branch padrao' }
    if ($tipo -eq 'BranchRemota') {
        $local=Git $repo @('rev-parse','--verify',('refs/heads/' + $nome))
        if ($local.OK -and -not (Ancestral $repo $local.Texto $d.Base)) { return 'Branch local homonima com commits pendentes' }
        $remota=Git $repo @('ls-remote','--heads','origin',('refs/heads/' + $nome))
        if (-not $remota.OK -or -not $remota.Texto.StartsWith($sha)) { return 'SHA remota mudou' }
    }
    return ''
}
function ValidarArquivo([object]$Item) {
    $repo=[string]$Item.Repositorio; $alvo=[string]$Item.Alvo; $tipo=[string]$Item.Tipo
    if (RepositorioProtegido $repo) { return 'GroveAI protegido integralmente' }
    if (-not (Abaixo $repo $Raiz) -or -not (Abaixo $alvo $repo)) { return 'Caminho fora do repositorio' }
    if (-not (Test-Path -LiteralPath (Join-Path $repo '.git')) -or -not (Test-Path -LiteralPath $alvo)) { return 'Caminho ou repo inexistente' }
    $obj=Get-Item -LiteralPath $alvo -Force
    if ($obj.Attributes -band [IO.FileAttributes]::ReparsePoint) { return 'Link/juncao nao permitido' }
    if (DentroDependencia $repo $alvo) { return 'Alvo interno de ambiente virtual/dependencias: bloqueado' }
    if ($obj.LastWriteTime.ToString('o') -ne [string]$Item.ModificadoEm) { return 'Data mudou desde o inventario' }
    if (Rastreado $repo $alvo) { return 'Arquivo ou pasta rastreada pelo Git' }
    if ($tipo -eq 'CachePasta') {
        if (-not $obj.PSIsContainer -or $obj.Name -notin $Caches -or -not (Ignorado $repo $alvo)) { return 'Cache nao confirmado como descartavel' }
    } elseif ($tipo -eq 'BuildPasta') {
        if (-not $obj.PSIsContainer -or $obj.Name -notin $Builds) { return 'Nao e build conhecido' }
    } elseif ($tipo -eq 'Dependencia') {
        if (-not $obj.PSIsContainer -or -not (DependenciaNome $obj.Name)) { return 'Nao e dependencia conhecida' }
    } elseif ($tipo -eq 'RelatorioTemporario') {
        if ($obj.PSIsContainer -or -not (RelatorioCandidato $obj.Name)) { return 'Nao e relatorio temporario conhecido' }
    } else { return 'Tipo de operacao nao permitido' }
    return ''
}
function Aplicacao {
    if (-not $Plano -or -not (Test-Path -LiteralPath $Plano -PathType Leaf)) { throw 'Informe -Plano com um plano.csv existente.' }
    if (-not (Abaixo $Plano $Saida)) { throw 'Plano deve estar dentro da pasta de auditorias.' }
    $linhas=@(Import-Csv -LiteralPath $Plano -Encoding UTF8 | Where-Object { $_.Autorizar -eq 'SIM' })
    Escrever "Operacoes autorizadas: $($linhas.Count)"
    if (-not $linhas.Count -and -not $LimparDocker) { return }
    Write-Host 'Confirme digitando APLICAR LIMPEZA:' -ForegroundColor Yellow
    if ((Read-Host) -cne 'APLICAR LIMPEZA') { throw 'Limpeza cancelada' }
    foreach($i in $linhas) {
        $problema=''
        try {
            $tipo=[string]$i.Tipo; $alvo=[string]$i.Alvo; $repo=[string]$i.Repositorio
            if (RepositorioProtegido $repo) { $problema='GroveAI protegido integralmente' }
            elseif ($tipo -in @('BranchLocal','BranchRemota')) {
                if ($tipo -eq 'BranchRemota' -and -not $ExcluirRemotas) { $problema='Exige -ExcluirRemotas' }
                else { $problema=ValidarBranch $i }
                if (-not $problema -and $tipo -eq 'BranchLocal') {
                    if (-not (Git $repo @('branch','-d','--',$alvo)).OK) { $problema='git branch -d falhou' }
                }
                if (-not $problema -and $tipo -eq 'BranchRemota') {
                    Write-Host "Remota $repo / $alvo. Digite EXCLUIR REMOTA:" -ForegroundColor Yellow
                    if ((Read-Host) -cne 'EXCLUIR REMOTA') { $problema='Nao confirmada' }
                    else {
                        $lease='--force-with-lease=refs/heads/' + $alvo + ':' + [string]$i.SHA
                        if (-not (Git $repo @('push',$lease,'origin',(':refs/heads/' + $alvo))).OK) { $problema='git push recusou exclusao' }
                    }
                }
            } else {
                $problema=ValidarArquivo $i
                if (-not $problema) { Remove-Item -LiteralPath $alvo -Recurse -Force -ErrorAction Stop }
            }
        } catch { $problema=$_.Exception.Message }
        $resultado=if ($problema) { 'PULADO/ERRO' } else { 'EXCLUIDO' }
        $script:Execucoes.Add([pscustomobject]@{ Tipo=$i.Tipo; Repositorio=$i.Repositorio; Alvo=$i.Alvo; Resultado=$resultado; Motivo=$problema })
        Escrever "$resultado $($i.Tipo) $($i.Alvo) $problema"
    }
    Csv $script:Execucoes.ToArray() (Join-Path $ExecDir 'execucao.csv') 'Tipo,Repositorio,Alvo,Resultado,Motivo'
}
function DockerRelatorio([string]$Nome) {
    if (-not (Get-Command docker -ErrorAction SilentlyContinue)) { Escrever 'Docker nao instalado.'; return $false }
    if (-not (Executar 'docker' @('info','--format','{{.ServerVersion}}')).OK) { Escrever 'Docker daemon indisponivel.'; return $false }
    @(
        'ESPACO:',(Executar 'docker' @('system','df')).Texto,
        'CONTAINERS:',(Executar 'docker' @('ps','-a')).Texto,
        'COMPOSE:',(Executar 'docker' @('compose','ls','--all')).Texto,
        'IMAGENS:',(Executar 'docker' @('image','ls')).Texto,
        'VOLUMES (NAO EXCLUIR):',(Executar 'docker' @('volume','ls')).Texto
    ) | Set-Content -LiteralPath (Join-Path $ExecDir ('docker-' + $Nome + '.txt')) -Encoding UTF8
    return $true
}
function DockerLimpeza {
    if (-not (DockerRelatorio 'antes')) { return }
    Write-Host 'AVISO: Docker prune atua no computador INTEIRO, nao somente nos repositorios Furriel.' -ForegroundColor Yellow
    Write-Host 'Nao apaga volumes, containers ativos nem imagens em uso.' -ForegroundColor Yellow
    Write-Host 'Digite LIMPAR DOCKER GLOBAL:' -ForegroundColor Yellow
    if ((Read-Host) -cne 'LIMPAR DOCKER GLOBAL') { Escrever 'Docker cancelado'; return }
    $filtro='until=' + ($DiasDocker * 24) + 'h'
    foreach($alvo in @('container','image','builder')) {
        $r=Executar 'docker' @($alvo,'prune','--force','--filter',$filtro)
        Escrever "docker $alvo prune: OK=$($r.OK); $($r.Texto)"
    }
    [void](DockerRelatorio 'depois')
}
function Compactar {
    try {
        $zip=$ExecDir + '.zip'
        Compress-Archive -Path (Join-Path $ExecDir '*') -DestinationPath $zip -Force -CompressionLevel Optimal
        Write-Host "ZIP para enviar ao ChatGPT: $zip" -ForegroundColor Cyan
    } catch {
        # PowerShell 5.1 suporta -Path com wildcard de forma mais consistente.
        try {
            Compress-Archive -Path (Join-Path $ExecDir '*') -DestinationPath ($ExecDir + '.zip') -Force
            Write-Host ('ZIP para enviar ao ChatGPT: ' + $ExecDir + '.zip') -ForegroundColor Cyan
        } catch { Escrever "ZIP indisponivel: $($_.Exception.Message)" }
    }
}
try {
    if ($LimparDocker) { throw 'Docker prune global desativado: nao ha como garantir preservacao de imagens/caches do GroveAI. Use somente -IncluirDocker para inventario.' }
    Escrever "Inicio; Aplicar=$Aplicar; Raiz=$Raiz"
    if ($Aplicar) { Aplicacao } else { Auditoria }
    if ($IncluirDocker -or $LimparDocker) {
        if ($Aplicar -and $LimparDocker) { DockerLimpeza }
        else { [void](DockerRelatorio 'inventario') }
    }
    Compactar
    Write-Host "RESULTADO: $ExecDir" -ForegroundColor Cyan
} catch {
    Escrever "FALHA: $($_.Exception.Message)"
    Compactar
    throw
}