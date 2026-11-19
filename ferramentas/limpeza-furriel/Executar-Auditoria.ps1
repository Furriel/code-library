#Requires -Version 5.1
<#
Executa SOMENTE a auditoria. Nenhum item sera excluido.
Caminho esperado: D:\GitHub\Furriel\code-library\ferramentas\limpeza-furriel
#>
[CmdletBinding()]
param(
    [string]$Raiz = 'D:\GitHub\Furriel',
    [string]$Saida = 'D:\GitHub\_Auditorias_Furriel',
    [int]$DiasMinimos = 7,
    [switch]$MedirTamanhos,
    [switch]$SemDocker,
    [switch]$SemFetch
)
$ErrorActionPreference='Stop'
$script=Join-Path $PSScriptRoot 'Limpar-Furriel.ps1'
if (-not (Test-Path -LiteralPath $script)) { throw "Script ausente: $script" }
$opcoes=@{
    Raiz=$Raiz
    Saida=$Saida
    DiasMinimos=$DiasMinimos
    IncluirRelatorios=$true
    IncluirBuilds=$true
    IncluirDependencias=$true
    IncluirDocker=(-not $SemDocker)
    MedirTamanhos=[bool]$MedirTamanhos
    SemFetch=[bool]$SemFetch
}
& $script @opcoes
if (-not $?) { throw 'Falha ao executar a auditoria.' }
