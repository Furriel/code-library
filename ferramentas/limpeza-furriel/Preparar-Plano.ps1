#Requires -Version 5.1
<#
.SYNOPSIS
Prepara plano revisado sem excluir arquivos ou branches.
.DESCRIPTION
Copia o plano de auditoria, selecionando somente categorias explicitamente
permitidas. O plano original nao e alterado. Exclusao exige -Aplicar separado.
#>
[CmdletBinding()]
param(
    [string]$Auditorias = 'D:\GitHub\_Auditorias_Furriel',
    [string]$PlanoOrigem,
    [switch]$IncluirBranchesLocais,
    [switch]$IncluirNodeModules,
    [switch]$IncluirBuilds,
    [switch]$IncluirAmbienteTreino,
    [switch]$PermitirRepositoriosComAlteracoes
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if (-not (Test-Path -LiteralPath $Auditorias -PathType Container)) { throw "Pasta nao encontrada: $Auditorias" }
if (-not $PlanoOrigem) {
    $candidatos=@(
        Get-ChildItem -LiteralPath $Auditorias -Directory -ErrorAction Stop |
        Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'plano.csv') } |
        Sort-Object LastWriteTime -Descending
    )
    if (-not $candidatos.Count) { throw 'Nenhum plano.csv encontrado.' }
    $PlanoOrigem=Join-Path $candidatos[0].FullName 'plano.csv'
}
$PlanoOrigem=(Resolve-Path -LiteralPath $PlanoOrigem).ProviderPath
$execucao=Split-Path -Parent $PlanoOrigem
$reposPath=Join-Path $execucao 'repositorios.csv'
if (-not (Test-Path -LiteralPath $reposPath)) { throw "repositorios.csv ausente: $execucao" }
$registros=@(Import-Csv -LiteralPath $PlanoOrigem -Encoding UTF8)
$repos=@(Import-Csv -LiteralPath $reposPath -Encoding UTF8)
$estado=@{}
foreach($r in $repos) { $estado[[string]$r.Repositorio]=$r }
function RepoConfiavel([string]$Nome) {
    if (-not $script:estado.ContainsKey($Nome)) { return $false }
    $r=$script:estado[$Nome]
    return $r.FetchOK -eq 'True' -and $r.ComparacaoConfiavel -eq 'True' -and $r.PRsVerificados -eq 'True'
}
function RepoAlterado([string]$Nome) {
    if (-not $script:estado.ContainsKey($Nome)) { return $true }
    return $script:estado[$Nome].AlteracoesLocais -ne 'False'
}
function ComLockLocal([string]$Caminho) {
    $pai=Split-Path -Parent $Caminho
    foreach($nome in @('package-lock.json','npm-shrinkwrap.json','pnpm-lock.yaml','yarn.lock','bun.lock','bun.lockb')) {
        if (Test-Path -LiteralPath (Join-Path $pai $nome) -PathType Leaf) { return $true }
    }
    return $false
}
$contagem=@{}
foreach($registro in $registros) {
    $tipo=[string]$registro.Tipo
    $repo=[string]$registro.Repositorio
    $alvo=[string]$registro.Alvo
    $aprovado=$false
    if ($tipo -eq 'CachePasta' -and $registro.Autorizar -eq 'SIM') {
        $aprovado=$true
    } elseif ($tipo -eq 'BranchLocal' -and $IncluirBranchesLocais -and (RepoConfiavel $repo) -and -not (RepoAlterado $repo)) {
        $aprovado=$true
    } elseif ($tipo -eq 'Dependencia' -and $IncluirNodeModules -and (Split-Path -Leaf $alvo) -eq 'node_modules') {
        if ((ComLockLocal $alvo) -and ((-not (RepoAlterado $repo)) -or $PermitirRepositoriosComAlteracoes)) {
            $aprovado=$true
        }
    } elseif ($tipo -eq 'BuildPasta' -and $IncluirBuilds) {
        if ((-not (RepoAlterado $repo)) -or $PermitirRepositoriosComAlteracoes) { $aprovado=$true }
    } elseif ($tipo -eq 'Dependencia' -and $IncluirAmbienteTreino -and (Split-Path -Leaf $alvo) -eq '.venv-train') {
        if ((Test-Path -LiteralPath (Join-Path $repo 'pyproject.toml')) -and
            (Test-Path -LiteralPath (Join-Path $repo 'requirements.txt'))) { $aprovado=$true }
    }
    # O usuario determinou preservar todo o GroveAI, inclusive planos antigos.
    if ($repo -match '(?i)(?:^|[\\/])GroveAI_SkinCancer(?:[\\/]|$)') { $aprovado=$false }
    # Remotas e logs nunca sao autorizados automaticamente.
    if ($tipo -in @('BranchRemota','RelatorioTemporario')) { $aprovado=$false }
    $registro.Autorizar=if($aprovado){'SIM'}else{'NAO'}
    if ($aprovado) {
        if (-not $contagem.ContainsKey($tipo)) { $contagem[$tipo]=0 }
        $contagem[$tipo]++
    }
}
$saida=Join-Path $execucao 'plano-revisado.csv'
if ([string]::Equals($PlanoOrigem,$saida,[StringComparison]::OrdinalIgnoreCase)) { throw 'Plano de entrada nao pode ser o revisado.' }
$registros | Export-Csv -LiteralPath $saida -Encoding UTF8 -NoTypeInformation
$permitidos=@($registros | Where-Object { $_.Autorizar -eq 'SIM' })
$bytes=($permitidos | ForEach-Object {
    $valor=0L
    if ([int64]::TryParse([string]$_.Bytes,[ref]$valor)) { $valor } else { 0L }
} | Measure-Object -Sum).Sum
if ($null -eq $bytes) { $bytes=0 }
$linhas=@(
    'PLANO REVISADO - NENHUMA EXCLUSAO EXECUTADA',
    "Origem: $PlanoOrigem",
    "Novo plano: $saida",
    "Registros totais: $($registros.Count)",
    "Autorizados: $($permitidos.Count)",
    "Espaco aproximado selecionado: $([math]::Round(([double]$bytes/1GB),3)) GiB",
    'Branches remotas e relatorios de evidencia: sempre NAO'
)
foreach($nome in $contagem.Keys) { $linhas+=(($nome + ': ') + $contagem[$nome]) }
if ($IncluirAmbienteTreino) {
    $linhas+='AVISO: .venv-train contem dependencias de treino. Reinstalacao pode ser demorada e nao reproduzir exatamente o ambiente.'
}
$linhas | Set-Content -LiteralPath (Join-Path $execucao 'RESUMO-PLANO-REVISADO.txt') -Encoding UTF8
$linhas | ForEach-Object { Write-Host $_ }
Write-Host ''
Write-Host 'PARA APLICAR, APOS REVISAR O CSV:' -ForegroundColor Yellow
Write-Host ('& "' + (Join-Path $PSScriptRoot 'Limpar-Furriel.ps1') + '" -Aplicar -Plano "' + $saida + '"')
