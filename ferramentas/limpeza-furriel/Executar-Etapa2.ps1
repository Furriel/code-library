#Requires -Version 5.1
<#
.SYNOPSIS
Recupera a auditoria anterior (pasta ou ZIP) e executa SOMENTE diagnosticos.
.DESCRIPTION
Corrige ausencia de D:\GitHub\_Auditorias_Furriel sem inventar resultados:
usa a pasta existente ou restaura ZIP identificado e validado. Nao exclui
branches, clones, arquivos, caches, containers ou volumes Docker.
#>
[CmdletBinding()]
param(
    [string]$Raiz = 'D:\GitHub\Furriel',
    [string]$Auditorias = 'D:\GitHub\_Auditorias_Furriel',
    [string]$IdAuditoria = '20261119-123728-20336',
    [string]$ZipAuditoria,
    [switch]$SomenteCopias
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if ($IdAuditoria -notmatch '^\d{8}-\d{6}-\d+$') {
    throw 'Id da auditoria invalido.'
}
if (-not (Test-Path -LiteralPath $Raiz -PathType Container)) {
    throw "Pasta de repositorios inexistente: $Raiz"
}
if (-not (Get-Command git.exe -ErrorAction SilentlyContinue)) {
    throw 'Git para Windows nao encontrado.'
}
$audDir=Join-Path $Auditorias $IdAuditoria
$obrigatorios=@('plano.csv','duplicados.csv','pastas-revisao.csv','repositorios.csv')
# Se a pasta de auditorias foi movida, localizar copia em locais provaveis.
if (-not (Test-Path -LiteralPath (Join-Path $audDir 'plano.csv') -PathType Leaf)) {
    $raizesPossiveis=@(
        (Join-Path $Raiz '_Auditorias_Furriel'),
        (Join-Path (Split-Path -Parent $Raiz) '_Auditorias_Furriel'),
        (Join-Path $env:USERPROFILE 'Downloads'),
        (Join-Path $env:USERPROFILE 'Documents'),
        (Join-Path $env:USERPROFILE 'Desktop')
    )
    foreach($basePossivel in @($raizesPossiveis | Select-Object -Unique)) {
        $possivel=Join-Path $basePossivel $IdAuditoria
        if (Test-Path -LiteralPath (Join-Path $possivel 'plano.csv') -PathType Leaf) {
            $Auditorias=$basePossivel
            $audDir=$possivel
            Write-Host "Pasta de auditorias localizada: $Auditorias" -ForegroundColor Cyan
            break
        }
    }
}
$completo=$true
foreach($nome in $obrigatorios) {
    if (-not (Test-Path -LiteralPath (Join-Path $audDir $nome) -PathType Leaf)) {
        $completo=$false
    }
}
if (-not $completo) {
    $nomedoZip=$IdAuditoria + '.zip'
    $lugares=@(
        $Auditorias,
        (Join-Path $Raiz '_Auditorias_Furriel'),
        $Raiz,
        (Split-Path -Parent $Raiz),
        (Join-Path $env:USERPROFILE 'Downloads'),
        (Join-Path $env:USERPROFILE 'Desktop'),
        (Get-Location).Path
    )
    if (-not $ZipAuditoria) {
        foreach($dir in @($lugares | Select-Object -Unique)) {
            if (-not (Test-Path -LiteralPath $dir -PathType Container)) { continue }
            $candidate=Join-Path $dir $nomedoZip
            if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                $ZipAuditoria=$candidate
                break
            }
        }
    }
    if (-not $ZipAuditoria -or -not (Test-Path -LiteralPath $ZipAuditoria -PathType Leaf)) {
        Write-Host "Auditoria original nao localizada: $audDir" -ForegroundColor Yellow
        Write-Host "Procure o ZIP $nomedoZip em Downloads ou outra pasta." -ForegroundColor Yellow
        Write-Host 'Use: & .\Executar-Etapa2.ps1 -ZipAuditoria "C:\CAMINHO\ARQUIVO.zip"'
        throw 'Nao foi encontrada auditoria completa nem ZIP para restauracao. Nenhuma limpeza ocorreu.'
    }
    if ((Split-Path -Leaf $ZipAuditoria) -ne $nomedoZip) {
        throw "ZIP informado nao corresponde a auditoria esperada: $nomedoZip"
    }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead($ZipAuditoria)
    try {
        foreach($nome in $obrigatorios) {
            $matching=@($zip.Entries | Where-Object { $_.FullName -ceq $nome })
            if ($matching.Count -ne 1) {
                throw "ZIP invalido: $nome nao aparece exatamente uma vez na raiz."
            }
            if ($matching[0].Length -gt 20MB) {
                throw "ZIP invalido: $nome excede 20 MiB."
            }
        }
        New-Item -ItemType Directory -Force -Path $Auditorias | Out-Null
        New-Item -ItemType Directory -Force -Path $audDir | Out-Null
        foreach($nome in $obrigatorios) {
            $destino=Join-Path $audDir $nome
            if (Test-Path -LiteralPath $destino -PathType Leaf) { continue }
            $entry=@($zip.Entries | Where-Object { $_.FullName -ceq $nome })[0]
            $entrada=$entry.Open()
            try {
                $saida=[IO.File]::Open($destino,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write)
                try { $entrada.CopyTo($saida) } finally { $saida.Dispose() }
            } finally { $entrada.Dispose() }
        }
    } finally { $zip.Dispose() }
    Write-Host "Auditoria restaurada em: $audDir" -ForegroundColor Cyan
}
foreach($nome in $obrigatorios) {
    if (-not (Test-Path -LiteralPath (Join-Path $audDir $nome) -PathType Leaf)) {
        throw "Auditoria incompleta apos restauracao: $nome"
    }
}
# Validar estrutura minima. Nao executa conteudo arbitrario dos CSVs.
$plan=@(Import-Csv -LiteralPath (Join-Path $audDir 'plano.csv') -Encoding UTF8)
if (-not $plan.Count -or -not ($plan[0].PSObject.Properties.Name -contains 'Tipo') -or
    -not ($plan[0].PSObject.Properties.Name -contains 'Repositorio')) {
    throw 'plano.csv vazio ou sem colunas esperadas.'
}
$base=$PSScriptRoot
Write-Host "Plano localizado: $audDir" -ForegroundColor Cyan
Write-Host 'Executando inventario de copias (somente leitura)...'
& (Join-Path $base 'Analisar-Copias.ps1') -Auditorias $Auditorias -AuditoriaAnterior $audDir
if (-not $?) { throw 'Falha em Analisar-Copias.ps1.' }
if ($SomenteCopias) {
    Write-Host 'Etapa de copias concluida. Nenhuma exclusao realizada.'
    return
}
if (-not (Get-Command gh.exe -ErrorAction SilentlyContinue)) {
    Write-Warning 'gh.exe nao encontrado. Auditoria de branches remotas nao iniciada; instale/autentique GitHub CLI.'
    Write-Host 'Nenhuma exclusao realizada.'
    return
}
Write-Host 'Executando inventario remoto (NENHUMA exclusao)...'
& (Join-Path $base 'Limpar-Remotas.ps1') -Raiz $Raiz -Auditorias $Auditorias -PlanoAuditoria (Join-Path $audDir 'plano.csv')
if (-not $?) { throw 'Falha em Limpar-Remotas.ps1.' }
Write-Host 'Concluido. Envie os ZIPs COPIAS-*.zip e REMOTAS-*.zip para revisao.' -ForegroundColor Cyan
