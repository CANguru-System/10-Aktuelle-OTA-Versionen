# 10-Aktuelle Versionen-  
CANguru-Decoder mit "Over The Air" Möglichkeit (siehe auch unter Rep 90)  

In diesem Repository werden nacheinander die CANguru-Decoder eingestellt, die dann direkt über das WLAN, ohne eine Verbindung zwischen PC und Decoder mit einem USB-Kabel herstellen zu müssen. Das ist insbesondere dann von Vorteil, wenn der Decoder irgendwo auf der Anlage eingebaut ist.  
NOCH NICHT FERTIG!!!!!  
Unabhängig vom Fortschritt der Anlage beschreibe ich hier schon einmal, wie das Ganze funktionieren wird. Im Internet findet man diverse Programme, mit deren Hilfe man OTA implementieren kann. Leider sind diese Beispiele auf die CANguru-Decoder nicht direkt anwendbar, da für OTA natürlich die Decoder am WLAN mit ihrem "Drahtlos-Ohr" lauschen müssen, um festzustellen, ob eventuell eine Neuprogrammierung ansteht. Aber gerade das ist in unserem Falle nicht möglich, da wir die "Ohren" unseres ESP32 für die ESPNow-Verbindungen einsetzen. Wir müssen uns also etwas anderes überlegen.
   
Wie ist also dafür vorzugehen?   
Zunächst ist das Programm Show_IP-Adress über USB auf das Board zu laden. Das Programm löscht alle Daten im EEPROM und listet anschließend alle möglichen WLAN-Verbindungen auf. Diese Angaben sieht man natürlich nur, wenn man beispielsweise TERA TERM startet. Wählen Sie dann durch Eingabe der korrespondierenden Ziffer ein WLAN aus und geben Sie das zugehörige Passwort ein. Dann versucht das Board sich in das ausgewählte WLAN einzuloggen. War das erfolgreich, wird die zugehörige IP-Adresse dieses Boards angezeigt.
Nun tragen Sie diese IP-Adresse in die Datei platformio.ini ein bei dem Programm, das Sie nun auf das Board laden wollen, beispielsweise 03-Tag-3-Gleisbesetztmelder-OTA. Starten Sie VS-Code, übersetzen das Programm und starten den Upload. Das Programm wird nun über das WLAN auf das Board geladen.
Das ist der Vorgang, um ein OTA-Programm zum ersten Mal aufs Board zu bringen.
  
Wenn Sie später im Betrieb Änderungen oder eine neue Version aufs Board laden wollen, gehen Sie anders vor. Dafür gibt es eine geänderte Version der CANguru-Bridge und des CANguru-Servers. Laden Sie beide herunter. Installieren Sie die Bridge über das USB-Kabel auf dem Board. Wenn Sie nun einen OTA-Prozess für ein OTA-Programm starten wollen, klicken Sie beim CANguru-Server auf den Reiter Konfiguration. Dort sind wie bisher alle angeschlossenen Decoder aufgelistet. Klicken Sie in der Listbox den gewünschten Decoder an. Rechts wird zur Kontrolle seine IP-Adresse angezeigt. Klicken Sie nun auf den Button "Over The Air (OTA)". Der zugehörige Decoder versucht nun sich ins WLAN einzuloggen und ist anschließend für das Laden eines neuen Programmes Over the air bereit. Diesen Ladevorgang starten Sie ganz normal aus VS-Code.  
  
Viel Erfolg!

