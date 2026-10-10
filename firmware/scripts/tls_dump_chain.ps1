# tls_dump_chain.ps1 — ODCZYT łańcucha certyfikatów TLS (Etap 2, przygotowanie).
#
# Nic nie zmienia w firmware ani w repo. Tylko łączy się z hostem na porcie 443,
# buduje łańcuch zaufania w Windows i zapisuje certyfikaty jako PEM POZA repo
# (domyślnie %USERPROFILE%\ryby-tls). Nie zawiera żadnych sekretów.
#
# Uruchomienie (PowerShell 5, z katalogu repo):
#   powershell -ExecutionPolicy Bypass -File .\firmware\scripts\tls_dump_chain.ps1 -HostName firebaseio.com
#   powershell -ExecutionPolicy Bypass -File .\firmware\scripts\tls_dump_chain.ps1 -HostName api.telegram.org
#
# Wynik: pliki <host>_0.pem (liść), <host>_1.pem ... (pośrednie), ostatni = root.
# Dla ESP32 zwykle wystarczy root (lub pośredni, jeśli root nie ma w łańcuchu).
# Porównaj odciski SHA-256 między dwiema sieciami (dom / LTE) — mają być takie same
# dla korzenia, a liść może się zmieniać.

param(
  [Parameter(Mandatory = $true)] [string]$HostName,
  [int]$Port = 443,
  [string]$OutDir = (Join-Path $env:USERPROFILE "ryby-tls")
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$tcp = New-Object System.Net.Sockets.TcpClient($HostName, $Port)
try {
  # Walidacja wyłączona TYLKO na czas odczytu: łańcuch i tak budujemy niżej i sami go oglądamy.
  $cb = [System.Net.Security.RemoteCertificateValidationCallback]{ param($s, $c, $ch, $e) $true }
  $ssl = New-Object System.Net.Security.SslStream($tcp.GetStream(), $false, $cb)
  $ssl.AuthenticateAsClient($HostName)

  $leaf = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($ssl.RemoteCertificate)
  $chain = New-Object System.Security.Cryptography.X509Certificates.X509Chain
  $chain.ChainPolicy.RevocationMode = [System.Security.Cryptography.X509Certificates.X509RevocationMode]::Online
  [void]$chain.Build($leaf)

  Write-Host ("Host: {0}:{1}  protokol: {2}" -f $HostName, $Port, $ssl.SslProtocol)
  Write-Host ("Status łańcucha Windows: " + (($chain.ChainStatus | ForEach-Object { $_.Status }) -join ", "))

  $sha = [System.Security.Cryptography.SHA256]::Create()
  $i = 0
  foreach ($el in $chain.ChainElements) {
    $c = $el.Certificate
    $b64 = [Convert]::ToBase64String($c.RawData)
    # PEM: linie po 64 znaki (standard)
    $lines = New-Object System.Collections.Generic.List[string]
    for ($p = 0; $p -lt $b64.Length; $p += 64) {
      $len = [Math]::Min(64, $b64.Length - $p)
      $lines.Add($b64.Substring($p, $len))
    }
    $pem = "-----BEGIN CERTIFICATE-----`r`n" + ($lines -join "`r`n") + "`r`n-----END CERTIFICATE-----`r`n"
    $file = Join-Path $OutDir ("{0}_{1}.pem" -f $HostName, $i)
    [System.IO.File]::WriteAllText($file, $pem)

    $fp = ([BitConverter]::ToString($sha.ComputeHash($c.RawData))).Replace("-", "")
    Write-Host ("[{0}] {1}" -f $i, $c.Subject)
    Write-Host ("     wystawca : {0}" -f $c.Issuer)
    Write-Host ("     wygasa   : {0}" -f $c.NotAfter.ToString("yyyy-MM-dd"))
    Write-Host ("     SHA-256  : {0}" -f $fp)
    Write-Host ("     plik     : {0}" -f $file)
    $i++
  }
  Write-Host ""
  Write-Host "Zapisano $i certyfikat(ów) w: $OutDir (poza repo — nie commituj)."
}
finally {
  if ($ssl) { $ssl.Dispose() }
  $tcp.Close()
}
