# 10-Aktuelle Versionen-  
CANguru-Decoder mit "Over The Air" Möglichkeit (siehe auch unter Rep 90)  

In diesem Repository werden alle CANguru-Decoder aus dem Buch eingestellt, die dann direkt über das WLAN, ohne eine Verbindung zwischen PC und Decoder mit einem USB-Kabel herstellen zu müssen. Das ist insbesondere dann von Vorteil, wenn der Decoder irgendwo auf der Anlage eingebaut ist. Weiterhin sind Verbesserungen oder teilweise auch kleine Unzulänglichkeiten in der Software beseitigt.
Im Folgenden wird dargestellt, wie die Software auf die Decoder aufgebracht wird.  
Die CANguru-Bridge spielt eine Sonderrolle, da sie nicht OTA-fähig ist. Alle anderen werden auf die gleiche Weise behandelt. Deshalb zunächst die  
CANguru-Bridge  
Die CANguru-Brdige wird immer mit dem flashtool bedampft. Wie das geht, steht in dem gleichnamigen Repsitory. Die Dateien liegen in dem Verzeichnis Files.
Alle anderen Decoder   
Im Internet findet man diverse Programme, mit deren Hilfe man OTA implementieren kann. Leider sind diese Beispiele auf die CANguru-Decoder nicht direkt anwendbar, da für OTA natürlich die Decoder am WLAN mit ihrem "Drahtlos-Ohr" lauschen müssen, um festzustellen, ob eventuell eine Neuprogrammierung ansteht. Aber gerade das ist in unserem Falle nicht möglich, da wir die "Ohren" unseres ESP32 für die ESPNow-Verbindungen einsetzen. Wir müssen uns also etwas anderes überlegen.  
Wie ist also dafür vorzugehen?   
Zunächst ist das Programm Show_IP-Adress über USB mit dem flashtool auf das Board zu laden. Das Programm löscht bei Bedarf alle Daten im EEPROM und listet anschließend alle möglichen WLAN-Verbindungen auf. Diese Angaben sieht man, wenn man ein Terminalprogramm wie beispielsweise TERA TERM startet. Wählen Sie dann durch Eingabe der korrespondierenden Ziffer ein WLAN aus und geben Sie das zugehörige Passwort ein. Nun versucht das Board, sich in das ausgewählte WLAN einzuloggen. War das erfolgreich, wird die zugehörige IP-Adresse dieses Boards angezeigt.  
Nun starten Sie das Programm ESP32_GUI_Programmer. Dieses Programm finden Sie im Verzeichnis 99-OTA_GUI\application.windows64, wenn Sie einen Windows-Rechner mit einem 64-Bit-System nutzen. Ansonsten gibt es auch eine Anwendung für 32-Bit und Linux. Eventuell müssen Sie beim ersten Start auch ein Java-Laufzeitsystem installieren. Eine entsprechende Datei finden Sie auch in dem Verzeichnis 99-OTA_GUI. Es erscheint zunächst ein Fenster, mit dem Sie die aktuelle Software, die auf das Board soll, auswählen. Diese finden Sie immer in einem Verzeichnis mit dem Namen OTA_BinFile. Danach kommt ein Fenster, in das Sie die IP-Adresse des Decoders eintragen. Anschließend betätigen Sie den Knopf Upload. Nun wird diese Software auf das Board übertragen.    
Das ist der Vorgang, um ein OTA-Programm zum ersten Mal aufs Board zu bringen.
  
Wenn Sie später im Betrieb Änderungen oder eine neue Version aufs Board laden wollen, gehen Sie anders vor. Dafür gibt es eine geänderte Version der CANguru-Bridge und des CANguru-Servers. Laden Sie beide herunter. Installieren Sie die Bridge über das USB-Kabel auf dem Board. Wenn Sie nun einen OTA-Prozess für ein OTA-Programm starten wollen, klicken Sie beim CANguru-Server auf den Reiter Konfiguration. Dort sind wie bisher alle angeschlossenen Decoder aufgelistet. Klicken Sie in der Listbox den gewünschten Decoder an. Rechts wird zur Kontrolle seine IP-Adresse angezeigt. Klicken Sie nun auf den Button "Over The Air (OTA)". Der zugehörige Decoder versucht nun sich ins WLAN einzuloggen und ist anschließend für das Laden eines neuen Programmes Over the air bereit. Diesen Ladevorgang starten Sie nun mit dem ESP32_GUI_Programmer wie oben beschrieben.  
  
An dieser Stelle muss ich mich bei dem Entwickler des ESP32_GUI_Programmers Kevin Darrah bedanken. Die Anwendung dieses Programmes ist eine große Erleichterung bei der Installation neuer Versionen. Vielen Dank!
Viel Erfolg!

