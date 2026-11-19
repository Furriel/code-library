#Requires -Version 5.1
<#
.SYNOPSIS
Audita branches remotas obsoletas e, somente sob autorizacao, remove as elegiveis.
.DESCRIPTION
Preserva integralmente GroveAI, workboard/B03/B04, backups, branches ativas e
projetos de pesquisa em andamento. Nao toca em arquivos locais, worktrees,
Docker nem volumes. Exige PRs verificaveis, ancestralidade e force-with-lease.
#>
[CmdletBinding()]
param(
    [string]$Raiz = 'D:\GitHub\Furriel',
    [string]$Auditorias = 'D:\GitHub\_Auditorias_Furriel',
    [string]$PlanoAuditoria,
    [string]$PlanoRemotas,
    [int]$DiasMinimos = 14,
    [switch]$Aplicar
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$env:GIT_TERMINAL_PROMPT = '0'
if ($DiasMinimos -lt 14) { throw 'Idade minima de 14 dias para limpeza de branches remotas.' }
if (-not (Get-Command git.exe -ErrorAction SilentlyContinue)) { throw 'Git para Windows indisponivel.' }
if (-not (Get-Command gh.exe -ErrorAction SilentlyContinue)) { throw 'GitHub CLI (gh) obrigatorio para checar PRs e branch principal.' }
if (-not (Test-Path -LiteralPath $Raiz -PathType Container)) { throw "Raiz ausente: $Raiz" }
if (-not (Test-Path -LiteralPath $Auditorias -PathType Container)) { throw "Auditorias ausentes: $Auditorias" }
$Raiz=(Resolve-Path -LiteralPath $Raiz).ProviderPath.TrimEnd('\')
$Auditorias=(Resolve-Path -LiteralPath $Auditorias).ProviderPath.TrimEnd('\')
$RelatorioDir=Join-Path $Auditorias ('REMOTAS-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $PID)
New-Item -ItemType Directory -Path $RelatorioDir -Force | Out-Null
$Log=Join-Path $RelatorioDir 'execucao.log'
$BloquearRepos=@(
    'furriel/groveai_skincancer',
    'furriel/workboard',
    'furriel/artigos',
    'furriel/review',
    'furriel/energy_analyzer'
)
$Avaliadas=[Collections.Generic.List[object]]::new()
$Executadas=[Collections.Generic.List[object]]::new()
function Mensagem([string]$Msg) {
    $linha='[{0}] {1}' -f (Get-Date -Format 's'),$Msg
    Write-Host $linha
    Add-Content -LiteralPath $script:Log -Value $linha -Encoding UTF8
}
function Rodar([string]$Programa,[string[]]$Argumentos) {
    $anterior=$ErrorActionPreference
    $ErrorActionPreference='Continue'
    try {
        $saida=@(& $Programa @Argumentos 2>$null)
        $codigo=$LASTEXITCODE
    } finally { $ErrorActionPreference=$anterior }
    return [pscustomobject]@{ OK=($codigo -eq 0); Texto=(@($saida) -join [Environment]::NewLine).Trim() }
}
function G([string]$Pasta,[string[]]$ArgsGit) { return Rodar 'git.exe' (@('-C',$Pasta) + $ArgsGit) }
function EhDentro([string]$Pasta,[string]$Base) {
    try {
        $x=[IO.Path]::GetFullPath($Pasta).TrimEnd('\')
        return $x.StartsWith($Base + '\',[StringComparison]::OrdinalIgnoreCase)
    } catch { return $false }
}
function NomeRemoto([string]$Pasta) {
    $u=G $Pasta @('remote','get-url','origin')
    if (-not $u.OK) { return '' }
    if ($u.Texto -match '(?i)github\.com[:/]Furriel/([A-Za-z0-9_.-]+?)(?:\.git)?$') {
        return ('Furriel/' + $Matches[1])
    }
    return ''
}
function Protegida([string]$Nome,[string]$Principal) {
    if ($Nome -eq $Principal) { return $true }
    if ($Nome -match '(?i)^(main|master|dev|develop|development|prod|production|staging|stable|release|v[0-9]+(?:\.[0-9]+)*)$') { return $true }
    if ($Nome -match '(?i)(^|[/_-])(backup|bkp|snapshot|legacy|archive|rescue|rollback|release|homolog|staging|production|prod)([/_-]|$)') { return $true }
    return $false
}
function Validar([object]$Item) {
    $pasta=[string]$Item.Repositorio
    $repo=[string]$Item.GitHub
    $nome=[string]$Item.Branch
    $sha=[string]$Item.SHA
    $r=[ordered]@{ Status='BLOQUEADA'; Motivo=''; DataCommit=''; IdadeDias=''; BranchPadrao='' }
    if (-not (EhDentro $pasta $Raiz)) { $r.Motivo='Repositorio fora da raiz'; return [pscustomobject]$r }
    if (-not (Test-Path -LiteralPath (Join-Path $pasta '.git'))) { $r.Motivo='Git local nao encontrado'; return [pscustomobject]$r }
    if ($pasta -match '(?i)GroveAI_SkinCancer' -or $repo.ToLowerInvariant() -in $BloquearRepos) {
        $r.Motivo='Repositorio protegido (GroveAI, Silico ou pesquisa ativa)'
        return [pscustomobject]$r
    }
    if ($repo -ne (NomeRemoto $pasta)) { $r.Motivo='URL origin divergente'; return [pscustomobject]$r }
    if ($repo -notmatch '^Furriel/[A-Za-z0-9_.-]+$') { $r.Motivo='Owner nao permitido'; return [pscustomobject]$r }
    if ($sha -notmatch '^[0-9a-f]{40}$') { $r.Motivo='SHA invalido'; return [pscustomobject]$r }
    if ($nome -match '[\s:]' -or $nome -notmatch '^[A-Za-z0-9][A-Za-z0-9._/-]*$') { $r.Motivo='Nome invalido'; return [pscustomobject]$r }
    $main=Rodar 'gh.exe' @('repo','view',$repo,'--json','defaultBranchRef','--jq','.defaultBranchRef.name')
    if (-not $main.OK -or -not $main.Texto) { $r.Motivo='Branch principal nao verificavel'; return [pscustomobject]$r }
    $principal=$main.Texto.Trim()
    $r.BranchPadrao=$principal
    if (Protegida $nome $principal) { $r.Motivo='Branch reservada ou de recuperacao'; return [pscustomobject]$r }
    $fetch=G $pasta @('fetch','--prune','origin')
    if (-not $fetch.OK) { $r.Motivo='Falha no fetch'; return [pscustomobject]$r }
    $sujo=G $pasta @('status','--porcelain')
    if (-not $sujo.OK -or $sujo.Texto) { $r.Motivo='Alteracoes locais ou status indisponivel'; return [pscustomobject]$r }
    $base=G $pasta @('rev-parse','--verify',('refs/remotes/origin/' + $principal))
    if (-not $base.OK) { $r.Motivo='Branch principal nao localizada'; return [pscustomobject]$r }
    $head=G $pasta @('rev-parse','--verify',('refs/remotes/origin/' + $nome))
    if (-not $head.OK -or $head.Texto -ne $sha) { $r.Motivo='Branch movida, inexistente ou SHA divergente'; return [pscustomobject]$r }
    $remoto=G $pasta @('ls-remote','--heads','origin',('refs/heads/' + $nome))
    if (-not $remoto.OK -or -not $remoto.Texto.StartsWith($sha + [char]9)) { $r.Motivo='SHA remoto divergente'; return [pscustomobject]$r }
    if (-not (G $pasta @('merge-base','--is-ancestor',$sha,$base.Texto)).OK) {
        $r.Motivo='Commit nao incorporado por ancestralidade'; return [pscustomobject]$r
    }
    $wt=G $pasta @('worktree','list','--porcelain')
    if (-not $wt.OK) { $r.Motivo='Worktrees nao verificaveis'; return [pscustomobject]$r }
    if (@($wt.Texto -split '\r?\n' | Where-Object { $_ -ceq ('branch refs/heads/' + $nome) }).Count) {
        $r.Motivo='Branch em uso por worktree'; return [pscustomobject]$r
    }
    $prs=Rodar 'gh.exe' @('pr','list','--repo',$repo,'--state','open','--json','headRefName','--limit','1000')
    if (-not $prs.OK) { $r.Motivo='Nao foi possivel consultar PRs'; return [pscustomobject]$r }
    try { $abertos=@($prs.Texto | ConvertFrom-Json) }
    catch { $r.Motivo='Resposta invalida de PRs'; return [pscustomobject]$r }
    if ($abertos.Count -ge 1000) { $r.Motivo='Mais de 1000 PRs; checagem incompleta'; return [pscustomobject]$r }
    if (@($abertos | Where-Object { $_.headRefName -eq $nome }).Count) {
        $r.Motivo='PR aberto'; return [pscustomobject]$r
    }
    $commit=G $pasta @('show','-s','--format=%cI',$sha)
    if (-not $commit.OK) { $r.Motivo='Data de commit nao verificavel'; return [pscustomobject]$r }
    try {
        $data=[DateTimeOffset]::Parse($commit.Texto,[Globalization.CultureInfo]::InvariantCulture)
        $dias=([DateTimeOffset]::UtcNow-$data.ToUniversalTime()).TotalDays
    } catch { $r.Motivo='Data de commit invalida'; return [pscustomobject]$r }
    $r.DataCommit=$data.ToString('o')
    $r.IdadeDias=[math]::Round($dias,1)
    if ($dias -lt $DiasMinimos) { $r.Motivo="Commit recente: $([math]::Round($dias,1)) dias"; return [pscustomobject]$r }
    $r.Status='ELEGIVEL'
    $r.Motivo='Ancestral da principal, antigo, sem PR aberto, SHA conferido'
    return [pscustomobject]$r
}
function Exportar([object[]]$Itens,[string]$Arquivo,[string]$Vazio) {
    if ($Itens.Count) { $Itens | Export-Csv -LiteralPath $Arquivo -Encoding UTF8 -NoTypeInformation }
    else { Set-Content -LiteralPath $Arquivo -Value $Vazio -Encoding UTF8 }
}
function Finalizar {
    try {
        $zip=$RelatorioDir + '.zip'
        Compress-Archive -Path (Join-Path $RelatorioDir '*') -DestinationPath $zip -Force
        Write-Host "ZIP de evidencias: $zip" -ForegroundColor Cyan
    } catch { Mensagem "Falha ao compactar: $($_.Exception.Message)" }
}
try {
    if (-not $Aplicar) {
        if (-not $PlanoAuditoria) {
            $dirs=@(Get-ChildItem -LiteralPath $Auditorias -Directory |
                Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'plano.csv') } |
                Sort-Object LastWriteTime -Descending)
            if (-not $dirs.Count) { throw 'Nenhuma auditoria com plano.csv encontrada.' }
            $PlanoAuditoria=Join-Path $dirs[0].FullName 'plano.csv'
        }
        if (-not (Test-Path -LiteralPath $PlanoAuditoria -PathType Leaf)) { throw "Plano inexistente: $PlanoAuditoria" }
        $linhas=@(Import-Csv -LiteralPath $PlanoAuditoria -Encoding UTF8 |
            Where-Object { $_.Tipo -eq 'BranchRemota' })
        Mensagem "Candidaturas recebidas: $($linhas.Count); deduplicando entre clones."
        $mapeadas=[Collections.Generic.List[object]]::new()
        foreach($i in $linhas) {
            $repo=[string]$i.Repositorio
            if (-not (EhDentro $repo $Raiz) -or -not (Test-Path -LiteralPath (Join-Path $repo '.git'))) { continue }
            $slug=NomeRemoto $repo
            if (-not $slug) { continue }
            $mapeadas.Add([pscustomobject]@{ Repositorio=$repo; GitHub=$slug; Branch=$i.Alvo; SHA=$i.SHA })
        }
        $grupos=@($mapeadas | Group-Object -Property GitHub,Branch)
        foreach($grupo in $grupos) {
            $entradas=@($grupo.Group | Sort-Object { $_.Repositorio.Length })
            $item=$entradas[0]
            if (@($entradas | Select-Object -ExpandProperty SHA -Unique).Count -gt 1) {
                $r=[pscustomobject]@{ Status='BLOQUEADA'; Motivo='SHAs diferentes entre clones'; DataCommit=''; IdadeDias=''; BranchPadrao='' }
            } else { $r=Validar $item }
            $Avaliadas.Add([pscustomobject][ordered]@{
                Repositorio=$item.Repositorio; GitHub=$item.GitHub; Branch=$item.Branch;
                SHA=$item.SHA; DataCommit=$r.DataCommit; IdadeDias=$r.IdadeDias;
                BranchPadrao=$r.BranchPadrao; Status=$r.Status; Motivo=$r.Motivo;
                DuplicatasNoInventario=$entradas.Count;
                Autorizar=if($r.Status -eq 'ELEGIVEL'){'SIM'}else{'NAO'}
            })
            Mensagem "$($item.GitHub) / $($item.Branch): $($r.Status) — $($r.Motivo)"
        }
        $csv=Join-Path $RelatorioDir 'plano-remotas.csv'
        Exportar $Avaliadas.ToArray() $csv 'Repositorio,GitHub,Branch,SHA,DataCommit,IdadeDias,BranchPadrao,Status,Motivo,DuplicatasNoInventario,Autorizar'
        $elegiveis=@($Avaliadas | Where-Object { $_.Status -eq 'ELEGIVEL' })
        @(
            "Candidaturas do plano anterior: $($linhas.Count)",
            "Branches unicas avaliadas: $($Avaliadas.Count)",
            "Elegiveis: $($elegiveis.Count)",
            "Bloqueadas/preservadas: $($Avaliadas.Count-$elegiveis.Count)",
            "Commit minimo: $DiasMinimos dias",
            'GroveAI, Silico Board e projetos de pesquisa ativa excluidos da selecao.',
            'Nao houve nenhuma exclusao. Confira Autorizar em plano-remotas.csv.'
        ) | Set-Content -LiteralPath (Join-Path $RelatorioDir 'RESUMO.txt') -Encoding UTF8
        Mensagem "Plano: $csv"
    } else {
        if (-not $PlanoRemotas -or -not (Test-Path -LiteralPath $PlanoRemotas -PathType Leaf)) {
            throw 'Para aplicar, informe -PlanoRemotas com o arquivo plano-remotas.csv revisado.'
        }
        if (-not (EhDentro $PlanoRemotas $Auditorias)) { throw 'Plano fora da pasta de auditorias.' }
        $linhas=@(Import-Csv -LiteralPath $PlanoRemotas -Encoding UTF8 |
            Where-Object { $_.Autorizar -eq 'SIM' -and $_.Status -eq 'ELEGIVEL' })
        $identidades=@($linhas | Group-Object -Property GitHub,Branch)
        if ($identidades.Count -ne $linhas.Count) { throw 'Plano contem branches duplicadas: refaca auditoria.' }
        Mensagem "Branches autorizadas para revalidacao: $($linhas.Count)"
        foreach($grp in @($linhas | Group-Object -Property GitHub)) {
            Mensagem "$($grp.Name): $($grp.Count) referencias"
        }
        if (-not $linhas.Count) { Mensagem 'Nada a excluir.'; Finalizar; return }
        Write-Host 'Para APAGAR as branches REMOTAS listadas, digite EXCLUIR REMOTAS:' -ForegroundColor Yellow
        if ((Read-Host) -cne 'EXCLUIR REMOTAS') { throw 'Cancelado sem exclusoes.' }
        foreach($i in $linhas) {
            $validade=Validar $i
            $resultado='PULADA'
            $detalhes=$validade.Motivo
            if ($validade.Status -eq 'ELEGIVEL') {
                $lease='--force-with-lease=refs/heads/' + $i.Branch + ':' + $i.SHA
                $del=G ([string]$i.Repositorio) @('push',$lease,'origin',(':refs/heads/' + $i.Branch))
                if ($del.OK) { $resultado='EXCLUIDA'; $detalhes='Exclusao remota com lease de SHA' }
                else { $resultado='ERRO'; $detalhes='Git push recusado/indisponivel; nada forcado' }
            }
            $Executadas.Add([pscustomobject]@{
                GitHub=$i.GitHub; Branch=$i.Branch; SHA=$i.SHA;
                Resultado=$resultado; Motivo=$detalhes
            })
            Mensagem "$($i.GitHub) / $($i.Branch): $resultado — $detalhes"
        }
        Exportar $Executadas.ToArray() (Join-Path $RelatorioDir 'resultado-remotas.csv') 'GitHub,Branch,SHA,Resultado,Motivo'
        $s=@($Executadas | Where-Object { $_.Resultado -eq 'EXCLUIDA' })
        Mensagem "Final: $($s.Count)/$($linhas.Count) branches remotas excluidas."
    }
} catch {
    Mensagem "FALHA: $($_.Exception.Message)"
    throw
} finally {
    Finalizar
}
