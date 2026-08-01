' update_progress_stil.vbs
'
' Start update_progress.ps1 zonder dat er een consolevenster opflitst.
'
' Waarom dit bestand bestaat: de taak "ECLIPSE Dashboard" draaide
' powershell.exe met -WindowStyle Hidden. Dat verbergt het venster PAS nadat
' Windows het al heeft aangemaakt, dus je zag elke 15 minuten een flits.
' De nette oplossing (taak op S4U zetten, buiten de bureaubladsessie) vereist
' beheerdersrechten die Nathan niet heeft. wscript.exe heeft zelf geen
' console, en start hieronder PowerShell met vensterstijl 0 = verborgen.
' Resultaat: niets te zien.

Dim sh, cmd
Set sh = CreateObject("WScript.Shell")

cmd = "powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass " & _
      "-File ""C:\Dev\ECLIPSE_GDD\Tools\update_progress.ps1"""

' 0 = verborgen venster, False = niet wachten op afloop
sh.Run cmd, 0, False
