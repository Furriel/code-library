#Requires -Version 5.1
<#
.SYNOPSIS
Avalia pastas P10/B03 e SAPE/IFAEE para limpeza futura. SOMENTE LEITURA.
.DESCRIPTION
Mede tamanho e calcula SHA256 de todos os arquivos de cada pasta de residuos.
Detecta duplicatas exatas por caminho relativo e conteudo. Nao altera git,
worktrees, logs, arquivos, Docker ou branches. GroveAI nao e inspecionado.
#>
[CmdletBinding()]
param(
    [string]$Raiz='D:\GitHub\Furriel',
    [string]$Saida='D:\GitHub\_Auditorias_Furriel',
    [switch]$NaoCalcularHashes
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
if (-not (Test-Path -LiteralPath $Raiz -PathType Container)) { throw "Raiz inexistente: $Raiz" }
$Raiz=(Resolve-Path -LiteralPath $Raiz).ProviderPath.TrimEnd('\')
if ($Saida -eq $Raiz -or $Saida.StartsWith($Raiz + '\',[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Saida nao pode ser criada dentro da raiz.'
}
New-Item -ItemType Directory -Force -Path $Saida | Out-Null
$Saida=(Resolve-Path -LiteralPath $Saida).ProviderPath.TrimEnd('\')
$diretorio=Join-Path $Saida ('RESIDUOS-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $PID)
New-Item -ItemType Directory -Path $diretorio -Force | Out-Null
$pastas=[Collections.Generic.List[object]]::new()
$erros=[Collections.Generic.List[object]]::new()
$tipos=@{
    'P10_B03'='workboard-P10-*-B03'
    'IFAEE_LOGS'='IFAEE-*-LOGS'
    'IFAEE_P_LOGS'='IFAEE-P??-logs'
    'SAPE_LOGS'='SAPE-HOMOLOGACAO-LOGS'
}
$raizDirs=@(Get-ChildItem -LiteralPath $Raiz -Directory -Force -ErrorAction Stop)
foreach($dir in $raizDirs) {
    $categoria=''
    foreach($tipo in $tipos.Keys) {
        if ($dir.Name -like $tipos[$tipo]) { $categoria=$tipo;break }
    }
    if (-not $categoria) { continue }
    if ($dir.Attributes -band [IO.FileAttributes]::ReparsePoint) {
        $erros.Add([pscustomobject]@{ Pasta=$dir.FullName; Erro='Pasta e link/juncao - ignorada' }); continue
    }
    if ($dir.FullName -match '(?i)GroveAI_SkinCancer|SILICO|backup|rollback|release') { continue }
    Write-Host "Inspecionando $($dir.Name) ..." -ForegroundColor Cyan
    $arquivos=[Collections.Generic.List[object]]::new()
    $fila=[Collections.Generic.Queue[string]]::new()
    $fila.Enqueue($dir.FullName)
    $contemGit=$false
    $contemLinks=$false
    $falha=$false
    while($fila.Count) {
        $atual=$fila.Dequeue()
        $entradas=@()
        try { $entradas=@(Get-ChildItem -LiteralPath $atual -Force -ErrorAction Stop) }
        catch { $erros.Add([pscustomobject]@{ Pasta=$dir.FullName; Erro=("Leitura: " + $_.Exception.Message) });$falha=$true;continue }
        foreach($item in $entradas) {
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { $contemLinks=$true;continue }
            if ($item.Name -eq '.git') { $contemGit=$true;continue }
            if ($item.PSIsContainer) { $fila.Enqueue($item.FullName);continue }
            $relativo=$item.FullName.Substring($dir.FullName.Length).TrimStart('\','/')
            $arquivos.Add([pscustomobject]@{
                Relativo=$relativo; Caminho=$item.FullName;
                Bytes=[int64]$item.Length; ModificadoEm=$item.LastWriteTime
            })
        }
    }
    $qtd=$arquivos.Count
    $bytes=[long]0
    if ($qtd) { $bytes=[long]($arquivos | Measure-Object -Property Bytes -Sum).Sum }
    $hash=''
    if (-not $NaoCalcularHashes -and -not $contemGit -and -not $contemLinks -and -not $falha) {
        $linhas=[Collections.Generic.List[string]]::new()
        foreach($arq in @($arquivos | Sort-Object -Property Relativo)) {
            try {
                $h=Get-FileHash -LiteralPath $arq.Caminho -Algorithm SHA256 -ErrorAction Stop
                $linhas.Add(($arq.Relativo -replace '\\','/') + '|' + $arq.Bytes + '|' + $h.Hash.ToLowerInvariant())
            } catch {
                $falha=$true
                $erros.Add([pscustomobject]@{ Pasta=$dir.FullName; Erro=("Hash: " + $_.Exception.Message) })
            }
        }
        if (-not $falha) {
            $dados=[Text.Encoding]::UTF8.GetBytes(($linhas -join [char]10))
            $sha=[Security.Cryptography.SHA256]::Create()
            try { $hash=([BitConverter]::ToString($sha.ComputeHash($dados)) -replace '-','').ToLowerInvariant() }
            finally { $sha.Dispose() }
        }
    }
    $status=if($contemGit){'PRESERVAR_CONTEM_GIT'}
      elseif($contemLinks){'PRESERVAR_CONTEM_LINK'}
      elseif($falha){'PRESERVAR_ERRO_LEITURA'}
      else{'REVISAR_SEM_EXCLUSAO'}
    $pastas.Add([pscustomobject][ordered]@{
        Pasta=$dir.FullName; Categoria=$categoria; Arquivos=$qtd; Bytes=$bytes;
        MiB=[math]::Round(($bytes/1MB),2);
        ModificadoEm=$dir.LastWriteTime.ToString('o');
        ContemGit=$contemGit; ContemLinks=$contemLinks;
        HashCompleto=$hash; Status=$status
    })
}
$duplicatas=[Collections.Generic.List[object]]::new()
$plano=[Collections.Generic.List[object]]::new()
$grupos=@($pastas | Where-Object { $_.HashCompleto } | Group-Object -Property HashCompleto | Where-Object { $_.Count -gt 1 })
foreach($g in $grupos) {
    $itens=@($g.Group | Sort-Object -Property ModificadoEm -Descending)
    $guardada=$itens[0]
    for($i=1;$i -lt $itens.Count;$i++) {
        $item=$itens[$i]
        $duplicatas.Add([pscustomobject]@{
            Candidata=$item.Pasta; ReferenciaPreservada=$guardada.Pasta;
            HashCompleto=$g.Name; Bytes=$item.Bytes;
            Motivo='Conteudo e nomes relativos identicos; revisar se evidencias sao necessarias'
        })
        $plano.Add([pscustomobject]@{
            Pasta=$item.Pasta; Tipo='DUPLICATA_EXATA';
            Bytes=$item.Bytes; HashCompleto=$g.Name;
            ReferenciaPreservada=$guardada.Pasta;
            Autorizar='NAO'; Acao='NENHUMA_EXCLUSAO_ESTA_ETAPA'
        })
    }
}
function Csv([object[]]$Dados,[string]$Arquivo,[string]$Cabecalho) {
    if($Dados.Count){$Dados | Export-Csv -LiteralPath $Arquivo -NoTypeInformation -Encoding UTF8}
    else{Set-Content -LiteralPath $Arquivo -Value $Cabecalho -Encoding UTF8}
}
Csv $pastas.ToArray() (Join-Path $diretorio 'pastas.csv') 'Pasta,Categoria,Arquivos,Bytes,MiB,ModificadoEm,ContemGit,ContemLinks,HashCompleto,Status'
Csv $duplicatas.ToArray() (Join-Path $diretorio 'duplicatas-exatas.csv') 'Candidata,ReferenciaPreservada,HashCompleto,Bytes,Motivo'
Csv $plano.ToArray() (Join-Path $diretorio 'plano-revisao.csv') 'Pasta,Tipo,Bytes,HashCompleto,ReferenciaPreservada,Autorizar,Acao'
Csv $erros.ToArray() (Join-Path $diretorio 'erros.csv') 'Pasta,Erro'
$totais=[long]0
if($pastas.Count){$totais=[long]($pastas | Measure-Object -Property Bytes -Sum).Sum}
@(
    'AUDITORIA DE RESIDUOS - NENHUMA EXCLUSAO',
    "Pastas inspecionadas: $($pastas.Count)",
    "Tamanho total: $([math]::Round(($totais/1GB),3)) GiB",
    "Duplicatas exatas (pastas excedentes): $($duplicatas.Count)",
    "Erros: $($erros.Count)",
    'GroveAI, SILICO, backups e repositorios Git nao removidos.',
    'Nenhum plano e aplicavel automaticamente. A classificacao nao comprova que logs sao dispensaveis.',
    'Retornar o ZIP para decidir futura quarentena/limpeza.'
) | Set-Content -LiteralPath (Join-Path $diretorio 'RESUMO.txt') -Encoding UTF8
$zip=$diretorio+'.zip'
Compress-Archive -Path (Join-Path $diretorio '*') -DestinationPath $zip -Force
Write-Host "Relatorio: $diretorio"
Write-Host "ZIP: $zip" -ForegroundColor Cyan
