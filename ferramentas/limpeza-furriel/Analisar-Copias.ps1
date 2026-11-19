#Requires -Version 5.1
<#
.SYNOPSIS
Inspeciona clones, worktrees e pastas de validacao. SOMENTE LEITURA.
.DESCRIPTION
Nao exclui arquivos, nao executa git fetch e nao muda worktrees.
GroveAI e releases/backup do Silico Board continuam protegidos.
#>
[CmdletBinding()]
param(
    [string]$Auditorias = 'D:\GitHub\_Auditorias_Furriel',
    [string]$AuditoriaAnterior
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
if (-not (Get-Command git.exe -ErrorAction SilentlyContinue)) { throw 'Git para Windows necessario' }
if (-not (Test-Path -LiteralPath $Auditorias -PathType Container)) { throw "Pasta nao encontrada: $Auditorias" }
if (-not $AuditoriaAnterior) {
    $candidatas=@(Get-ChildItem -LiteralPath $Auditorias -Directory |
        Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'duplicados.csv') } |
        Sort-Object LastWriteTime -Descending)
    if (-not $candidatas.Count) { throw 'Auditoria anterior sem duplicados.csv' }
    $AuditoriaAnterior=$candidatas[0].FullName
}
$copiasPath=Join-Path $AuditoriaAnterior 'duplicados.csv'
$pastasPath=Join-Path $AuditoriaAnterior 'pastas-revisao.csv'
if (-not (Test-Path -LiteralPath $copiasPath) -or -not (Test-Path -LiteralPath $pastasPath)) { throw 'Inventario incompleto.' }
$saida=Join-Path $Auditorias ('COPIAS-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $PID)
New-Item -ItemType Directory -Path $saida -Force | Out-Null
$alvos=[Collections.Generic.List[object]]::new()
$visitadas=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($c in @(Import-Csv -LiteralPath $copiasPath -Encoding UTF8)) {
    if (-not [string]::IsNullOrWhiteSpace($c.Caminho) -and $visitadas.Add([string]$c.Caminho)) {
        $alvos.Add([pscustomobject]@{ Caminho=$c.Caminho; Tipo='Clone'; GitHub=$c.GitHub; BytesOrigem='' })
    }
}
foreach($p in @(Import-Csv -LiteralPath $pastasPath -Encoding UTF8)) {
    if (-not [string]::IsNullOrWhiteSpace($p.Caminho) -and $visitadas.Add([string]$p.Caminho)) {
        $alvos.Add([pscustomobject]@{ Caminho=$p.Caminho; Tipo='PastaValidacao'; GitHub=''; BytesOrigem=$p.Bytes })
    }
}
function GitRead([string]$dir,[string[]]$argsGit) {
    $prev=$ErrorActionPreference
    $ErrorActionPreference='Continue'
    try {
        $lines=@(& git.exe -C $dir @argsGit 2>$null)
        $exit=$LASTEXITCODE
    } finally { $ErrorActionPreference=$prev }
    return [pscustomobject]@{ OK=($exit -eq 0); Texto=(@($lines) -join [Environment]::NewLine).Trim() }
}
$dados=[Collections.Generic.List[object]]::new()
foreach($c in $alvos) {
    $p=[string]$c.Caminho
    if (-not (Test-Path -LiteralPath $p -PathType Container)) { continue }
    $item=Get-Item -LiteralPath $p -Force
    $protegida=($p -match '(?i)GroveAI_SkinCancer|SILICO-B03|SILICO-B04|backup|rollback|release') -or
        ([string]$c.GitHub -eq 'Furriel/workboard')
    $git=(Test-Path -LiteralPath (Join-Path $p '.git'))
    $branch='';$head='';$modificacoes='';$gitdir='';$comum='';$ahead='';$behind='';$worktrees=''
    if ($git) {
        $g=Get-Item -LiteralPath (Join-Path $p '.git') -Force
        $gitdir=if($g.PSIsContainer){'REPOSITORIO_INDEPENDENTE'}else{'WORKTREE_OU_GITFILE'}
        $branch=(GitRead $p @('branch','--show-current')).Texto
        $head=(GitRead $p @('rev-parse','HEAD')).Texto
        $st=GitRead $p @('status','--porcelain=v1','-uall')
        $modificacoes=if($st.OK){@($st.Texto -split '[\r\n]+' | Where-Object { $_ }).Count}else{'INDETERMINADO'}
        $comum=(GitRead $p @('rev-parse','--git-common-dir')).Texto
        $rem=(GitRead $p @('symbolic-ref','--quiet','refs/remotes/origin/HEAD')).Texto
        if ($rem) {
            $base=(GitRead $p @('rev-parse','--verify',$rem)).Texto
            if ($base) {
                $r=GitRead $p @('rev-list','--left-right','--count',($base + '...HEAD'))
                if ($r.OK -and $r.Texto -match '^(\d+)\s+(\d+)$') { $behind=$Matches[1];$ahead=$Matches[2] }
            }
        }
        $w=(GitRead $p @('worktree','list','--porcelain')).Texto
        $worktrees=@($w -split '[\r\n]+' | Where-Object { $_ -like 'worktree *' }) -join ' | '
    }
    $classificacao='REVISAR_MANUALMENTE_SEM_EXCLUSAO'
    if ($protegida) { $classificacao='PRESERVAR' }
    elseif ($gitdir -eq 'WORKTREE_OU_GITFILE') { $classificacao='WORKTREE_NAO_REMOVER_DIRETORIO' }
    elseif ($git -and ([string]$modificacoes -ne '0' -or [string]$ahead -ne '0')) {
        $classificacao='PRESERVAR_POR_ALTERACOES_OU_COMMITS_PENDENTES'
    }
    $dados.Add([pscustomobject][ordered]@{
        Caminho=$p; Tipo=$c.Tipo; GitHub=$c.GitHub;
        BytesMedidosAuditoria=$c.BytesOrigem; ModificadoEm=$item.LastWriteTime.ToString('o');
        Git=$git; TipoGit=$gitdir; Branch=$branch; HEAD=$head;
        Alteracoes=$modificacoes; Ahead=$ahead; Behind=$behind;
        GitCommonDir=$comum; Worktrees=$worktrees; Classificacao=$classificacao
    })
}
$csv=Join-Path $saida 'copias-e-validacoes.csv'
if ($dados.Count) { $dados | Export-Csv -LiteralPath $csv -Encoding UTF8 -NoTypeInformation }
else { Set-Content -LiteralPath $csv -Encoding UTF8 -Value 'Caminho,Tipo,GitHub,Classificacao' }
@(
    'RELATORIO SOMENTE LEITURA - NENHUM ARQUIVO EXCLUIDO',
    "Origem: $AuditoriaAnterior",
    "Pastas examinadas: $($dados.Count)",
    'GroveAI preservado integralmente; Silico B03/B04 e worktrees protegidos.',
    'Comparacoes Git locais podem precisar de atualizacao remota.',
    'Nao e seguro apagar clones apenas porque possuem o mesmo origin.'
) | Set-Content -LiteralPath (Join-Path $saida 'RESUMO.txt') -Encoding UTF8
$zip=$saida + '.zip'
Compress-Archive -Path (Join-Path $saida '*') -DestinationPath $zip -Force
Write-Host "Inventario: $csv" -ForegroundColor Cyan
Write-Host "ZIP: $zip" -ForegroundColor Cyan
