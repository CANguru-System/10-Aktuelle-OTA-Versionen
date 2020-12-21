
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using System.Windows.Forms;

namespace CANguruX
{
    public partial class Form1 : Form
    {
        enum CMD
        {
            toCAN, toClnt, toUDP, toTCP, fromCAN, fromClnt, fromUDP, fromTCP, fromUDP2CAN, fromTCP2CAN, fromTCP2Clnt, fromCAN2UDP, fromCAN2TCP,
            fromGW2Clnt, fromGW2CAN, toGW, fromGW
        };
        private TcpClient Client = null;
        Thread connectThread;
        NetworkStream stream;
        StreamReader reader;

        // This delegate enables asynchronous calls for setting
        // the text property on a TextBox control.
        // Delegates
        //
        delegate void UpdateProgressPingBarDelegate();
        UpdateProgressPingBarDelegate UpdateProgressPingBarMethod;
        //
        delegate void UpdateProgressMFXBarDelegate();
        UpdateProgressMFXBarDelegate UpdateProgressMFXBarMethod;
        //
        delegate String GetMyTextDelegate(Control ctrl);
        delegate void ChangeMyTextDelegate(TextBox ctrl, string text);

        // buffers for receiving and sending data
        byte[] GETCONFIG_RESPONSE = { 0x00, 0x58, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        //
        IniFile ini = new IniFile();

        Thread threadCAN;
        // UDP to CAN
        public UdpClient CANServer;
        public UdpClient CANClient;
        //
        bool Voltage;
        CQueue myQueue;
        Cnames names;
        CConfigStream ConfigStream;
        //
        private static System.Timers.Timer a1secTimer;
        byte elapsedsec;
        //
        private static System.Timers.Timer a1milliTimer;
        Int16 elapsedmillis;
        //
        private static System.Timers.Timer gettingConnectionTimer;
        Int16 elapsedmillis4Connection;
        // gleisbild
        int cntGleisbild = 0;
        //
        bool receivePINGInfos;
        byte[,] CANguruArr;
        byte[,] CANguruPINGArr;
        struct configStruct
        {
            public Label indicatorLabel;
            public NumericUpDown indicatorNumericUpDown;
            public ListBox indicatorListBox;
            public int minValue;
            public int maxValue;
            public int currValue;
            public Label unit;
        }
        configStruct[] CANguruConfigArr;
        struct structConfigControls
        {
            public byte cntControls;
            public Control[] controlLabel;
            public Control[] controlUnit;
            public Control[] controlValue;
            public Control[] controlChoice;
        }
        structConfigControls configControls;
        byte CANguruArrFilled;
        byte CANguruArrIndex;
        byte CANguruArrWorked;
        byte CANguruArrLine;
        byte lastSelectedItem;
        byte[] GFP_UID = { 0x00, 0x00, 0x00, 0x00 };
        const byte GFP = 0x10;
        const byte DEVTYPE_BASE = 0x50;
        const byte DEVTYPE_SERVO = 0x53;
        const byte DEVTYPE_RM = 0x54;
        const byte DEVTYPE_LIGHT = 0x55;
        const byte DEVTYPE_SIGNAL = 0x56;
        const byte DEVTYPE_LEDSIGNAL = 0x57;
        const byte DEVTYPE_CANFUSE = 0x58;
        const byte DEVTYPE_LastCANguru = 0x5F;
        //
        List<byte[]> WeichenList = new List<byte[]>();
        //
        static bool is_connected;
        bool emptyLokListIsGenerated;

        static bool verbose = true;
        static byte lastCMD = 0;

        private void DisableTab_DrawItem(object sender, DrawItemEventArgs e)
        {
            const byte spaces = 3;
            TabControl tabControl = sender as TabControl;
            TabPage page = tabControl.TabPages[e.Index];
            if (!page.Enabled)
            {
                //Draws disabled tab
                using (SolidBrush brush = new SolidBrush(SystemColors.GrayText))
                {
                    e.Graphics.DrawString(page.Text, page.Font, brush, e.Bounds.X + spaces, e.Bounds.Y + spaces);
                }
            }
            else
            {
                // Draws normal tab
                using (SolidBrush brush = new SolidBrush(page.ForeColor))
                {
                    e.Graphics.DrawString(page.Text, page.Font, brush, e.Bounds.X + spaces, e.Bounds.Y + spaces);
                }
            }
        }

        private void tabControl1_Selecting(object sender, TabControlCancelEventArgs e)
        {
            //Cancels click on disabled tab.
            if (!e.TabPage.Enabled)
                e.Cancel = true;
        }

        public Form1()
        {
            InitializeComponent();
            Voltage = false;
            btnVolt.Enabled = false;
            // Enable first tab
            this.Telnet.Enabled = true; // no casting required.
            // Disable additional tabs
            this.MFX.Enabled = false; // no casting required.
            this.Configuration.Enabled = false; // no casting required.
            this.tabControl1.Selecting += new TabControlCancelEventHandler(tabControl1_Selecting);
            this.tabControl1.DrawMode = TabDrawMode.OwnerDrawFixed;
            this.tabControl1.DrawItem += new DrawItemEventHandler(DisableTab_DrawItem);
            is_connected = false;
            emptyLokListIsGenerated = false;
            compress.bcompress = false;
            CANguruArr = new byte[Cnames.maxConfigLines, Cnames.lngFrame]; // [maxConfigLines+1][Cnames.lngFrame]
            CANguruPINGArr = new byte[Cnames.maxCANgurus, Cnames.lngFrame + 1 + 4]; // 4 für die IP-Adresse
                                                                                    // [maxCANgurus][maxConfigLines+1][Cnames.lngFrame]
                                                                                    //
                                                                                    // (common section's keys will overwrite)
                                                                                    // initialize the delegate here
            UpdateProgressPingBarMethod = new UpdateProgressPingBarDelegate(UpdateProgressPingBar);
            UpdateProgressMFXBarMethod = new UpdateProgressMFXBarDelegate(UpdateProgressMFXBar);
            this.progressBarPing.Visible = false;
            this.progressBarMfx.Visible = false;
            try
            {
                myQueue = new CQueue();
                names = new Cnames();
                // CANServer empfängt von CAN (Arduino-Gateway)
                CANServer = new UdpClient(Cnames.portinCAN);
                // CANClient sendet nach UDP (WDP)
                CANClient = new UdpClient();
                ConfigStream = new CConfigStream(names);
                // Determine whether the directory exists.
                string iniStr = string.Concat(Cnames.path, Cnames.ininame);
                bool fexists = File.Exists(iniStr);
                //                if (Directory.Exists(Cnames.path))
                if (fexists)
                {
                    ini.Load(iniStr);
                    //  Returns a KeyValue in a certain section
                    Cnames.IP_CAN = ini.GetKeyValue("IP-address", "IPCAN");
                    byte c = 0;
                    if (!byte.TryParse(ini.GetKeyValue("Neuanmeldezaehler", "Counter"), out c))
                        c = ConfigStream.getMinCounter();
                    if (c > ConfigStream.getMaxCounter())
                        c = ConfigStream.getMinCounter();
                    ConfigStream.setCounter(c);
                    byte n = 0;
                    if (!byte.TryParse(ini.GetKeyValue("Lok-Adresse", "LocID"), out n))
                        n = ConfigStream.getMinLocID();
                    if (n > ConfigStream.getMaxLocID())
                        n = ConfigStream.getMinLocID();
                    ConfigStream.setnextLocid(n);
                    byte v = 0;
                    if (!byte.TryParse(ini.GetKeyValue("Verbose", "verbose"), out v))
                        verbose = true;
                    else
                        verbose = v == 1;
                    if (verbose)
                    {
                        btnVerbose.Text = "Not verbose";
                    }
                    else
                    {
                        btnVerbose.Text = "Verbose";
                        verbose = false;
                    }
                    if (Voltage)
                    {
                        btnVolt.Text = "Gleisspannung AUS";
                    }
                    else
                    {
                        btnVolt.Text = "Gleisspannung AUS";
                        Voltage = false;
                    }
                }
                else
                {
                    // Try to create the directory.
                    DirectoryInfo di = Directory.CreateDirectory(Cnames.path);
                    ini.AddSection("IP-address").AddKey("IPCAN").Value = Cnames.IP_CAN;
                    ini.AddSection("Neuanmeldezaehler").AddKey("Counter").Value = "0";
                    ini.AddSection("Lok-Adresse").AddKey("LocID").Value = "1";
                    ini.AddSection("Verbose").AddKey("verbose").Value = "1";
                    ConfigStream.setCounter(ConfigStream.getMinCounter());
                    ConfigStream.setnextLocid(ConfigStream.getMinLocID());
                }
                //
                this.Load += Form1_Load;
            }
            catch (Exception e)
            {
                MessageBox.Show("InitializeException: " + e.Message);
            }
            buttonConnect.Enabled = true;
        }

        string getindicatorName(ref byte line, ref byte lfd)
        {
            String str = "";
            while (true)
            {
                // in der Zeile 2 steht der Name des Gerätes
                char ch = (char)CANguruArr[line, lfd + 5];
                if (ch == 0x00)
                {
                    lfd++;
                    if (lfd == 8)
                    {
                        lfd = 0;
                        line++;
                    }
                    return str;
                }
                str += ch;
                lfd++;
                if (lfd == 8)
                {
                    lfd = 0;
                    line++;
                }
            }
        }

        private void Form1_Load(object sender, System.EventArgs e)
        {
            byte[] MFX_LOCID = { 0x00, 0x50, 0x03, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] MFX_COUNTER = { 0x00, 0x00, 0x03, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00 };
            // Set the Minimum, Maximum, and initial Value.
            numCounter.Value = ConfigStream.getCounter();
            numCounter.Maximum = ConfigStream.getMaxCounter();
            numCounter.Minimum = ConfigStream.getMinCounter();
            //
            for (byte uid = 0; uid < 4; uid++)
            {
                MFX_COUNTER[5 + uid] = GFP_UID[uid];
            }
            MFX_COUNTER[11] = ConfigStream.getCounter();
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(MFX_COUNTER, Cnames.lngFrame);
            // Set the Minimum, Maximum, and initial Value.
            numLocID.Value = ConfigStream.getnextLocid();
            numLocID.Maximum = ConfigStream.getMaxLocID();
            numLocID.Minimum = ConfigStream.getMinLocID();
            // an die Bridge melden
            MFX_LOCID[5] = ConfigStream.getnextLocid();
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(MFX_LOCID, Cnames.lngFrame);
            //
            // Set the Minimum, Maximum, and initial Value.
            numUpDnDecNumber.Maximum = 255;
            numUpDnDecNumber.Minimum = 1;
            numUpDnDecNumber.Value = 1;
            //
            // Set the Minimum, Maximum, and initial Value.
            numUpDnDelay.Maximum = 15;
            numUpDnDelay.Minimum = 1;
            numUpDnDelay.Value = 10;
            //
            // CAN
            threadCAN = new Thread(new ThreadStart(fromCAN2UDP));
            threadCAN.IsBackground = true;
            threadCAN.Start();
        }

        int getIndicatorValue(byte pos)
        {
            return CANguruArr[0, pos + 5];
        }

        string read1ConfigChannel_DescriptionBlock(ref byte CgArrIndex, ref byte[] content)
        {
            if (CANguruArrWorked < CANguruArrFilled)
            {
                // letzte Zeile eingelesen
                // auswerten des Paketes
                // Eintrag für Listbox erzeugen
                byte line = 2;
                byte pos = 0;
                String entry = getindicatorName(ref line, ref pos);
                // Anzahl der folgenden Messwerte aus Zeile 2, Position 6
                CANguruPINGArr[CANguruArrWorked, Cnames.lngFrame] = CANguruArr[0, 0x06];
                // der Hashwert zur Unterscheidung aus der
                // aktuellen Zeile Position 2 und 3
                entry += "-" + String.Format("{0:X02}", content[0x02]);
                entry += String.Format("{0:X02}", content[0x03]);
                // in Listbox eintragen
                this.CANElemente.Invoke(new MethodInvoker(() => CANElemente.Items.Add(entry)));
                // ersten Messwert vorbereiten
                CANguruArrLine = 0;
                CANguruArrWorked++;
                getConfigData(CANguruArrWorked, CgArrIndex);
                return entry;
            }
            return "";
        }

        void readChoiceBlock(ref byte CgArrIndex)
        {
            int numberofLEDProgs = getIndicatorValue(2);
            // > 0 sind Messwertbeschreibungen
            byte maxIndex = CANguruPINGArr[CANguruArrWorked, Cnames.lngFrame];
            int xpos0 = 50;
            int xpos1 = 250;
            int yposDelta = 0;
            for (byte cc = 0; cc < CgArrIndex - 1; cc++)
            {
                if (CANguruConfigArr[cc].indicatorNumericUpDown != null)
                    yposDelta += 25;
                else
                    yposDelta += 65;
            }

            int ypos = 150 + yposDelta;
            byte line = 1;
            byte pos = 0;
            // Wertename
            Label cntrlLabel = new Label();
            CANguruConfigArr[CgArrIndex - 1].indicatorLabel = cntrlLabel;
            cntrlLabel.Text = getindicatorName(ref line, ref pos);
            cntrlLabel.Location = new Point(xpos0, ypos);
            ListBox choiceBox = new ListBox();
            CANguruConfigArr[CgArrIndex - 1].indicatorListBox = choiceBox;
            CANguruConfigArr[CgArrIndex - 1].indicatorNumericUpDown = null;
            for (int ch = 0; ch < numberofLEDProgs; ch++)
            {
                String choice = getindicatorName(ref line, ref pos);
                choiceBox.Items.Add(choice);
            }
            choiceBox.Location = new Point(xpos1, ypos);
            choiceBox.Height = 60;
            // Label anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(cntrlLabel)));
            configControls.controlLabel[CgArrIndex - 1] = cntrlLabel;
            // choice anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(choiceBox)));
            configControls.controlChoice[CgArrIndex - 1] = choiceBox;
            // auswählen
            int select = getIndicatorValue(3);
            this.Configuration.Invoke(new MethodInvoker(() => choiceBox.SetSelected(select, true)));
            CANguruArrLine = 0;
            if (CgArrIndex < maxIndex)
            // letzte Zeile eingelesen
            {
                CgArrIndex++;
                getConfigData(CANguruArrWorked, CgArrIndex);
                return;
            }
            if (CgArrIndex == maxIndex)
            {
                // letzter Messwert eingelesen
                CgArrIndex = 0;
                lastSelectedItem = CANguruArrWorked;
                return;
            }
        }
        void readValueBlock(ref byte CgArrIndex)
        {
            // > 0 sind Messwertbeschreibungen
            byte maxIndex = CANguruPINGArr[CANguruArrWorked, Cnames.lngFrame];
            int xpos0 = 50;
            int xpos1 = 250;
            int ypos = 120 + CgArrIndex * 25;
            int w = 50;
            byte line = 1;
            byte pos = 0;
            // Wertename
            Label cntrlLabel = new Label();
            CANguruConfigArr[CgArrIndex - 1].indicatorLabel = cntrlLabel;
            cntrlLabel.Text = getindicatorName(ref line, ref pos);
            cntrlLabel.Location = new Point(xpos0, ypos);
            // Anfangsbezeichnung
            String notused = getindicatorName(ref line, ref pos);
            // Endebezeichnung
            notused = getindicatorName(ref line, ref pos);
            // Einheit
            Label unitLabel = new Label();
            CANguruConfigArr[CgArrIndex - 1].unit = unitLabel;
            unitLabel.Text = getindicatorName(ref line, ref pos);
            unitLabel.Location = new Point(xpos1 + w + 10, ypos);
            // Wert
            NumericUpDown ctrlNumericUpDown = new NumericUpDown();
            ctrlNumericUpDown.Width = w;
            CANguruConfigArr[CgArrIndex - 1].indicatorNumericUpDown = ctrlNumericUpDown;
            CANguruConfigArr[CgArrIndex - 1].indicatorListBox = null;
            ctrlNumericUpDown.Location = new Point(xpos1, ypos);
            // values big endian
            CANguruConfigArr[CgArrIndex - 1].minValue = (getIndicatorValue(2) << 8) + getIndicatorValue(3);
            ctrlNumericUpDown.Minimum = CANguruConfigArr[CgArrIndex - 1].minValue;
            CANguruConfigArr[CgArrIndex - 1].maxValue = (getIndicatorValue(4) << 8) + getIndicatorValue(5);
            ctrlNumericUpDown.Maximum = CANguruConfigArr[CgArrIndex - 1].maxValue;
            CANguruConfigArr[CgArrIndex - 1].currValue = (getIndicatorValue(6) << 8) + getIndicatorValue(7);
            ctrlNumericUpDown.Value = CANguruConfigArr[CgArrIndex - 1].currValue;
            // Label anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(cntrlLabel)));
            configControls.controlLabel[CgArrIndex - 1] = cntrlLabel;
            // Einheit anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(unitLabel)));
            configControls.controlUnit[CgArrIndex - 1] = unitLabel;
            // Wert anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(ctrlNumericUpDown)));
            configControls.controlValue[CgArrIndex - 1] = ctrlNumericUpDown;
            CANguruArrLine = 0;
            if (CgArrIndex < maxIndex)
            // letzte Zeile eingelesen
            {
                CgArrIndex++;
                getConfigData(CANguruArrWorked, CgArrIndex);
                return;
            }
            if (CgArrIndex == maxIndex)
            {
                // letzter Messwert eingelesen
                CgArrIndex = 0;
                lastSelectedItem = CANguruArrWorked;
                return;
            }
        }
        void read1ConfigChannel_ValueBlock(ref byte CgArrIndex)
        {
            int ptr = getIndicatorValue(1);
            if (ptr == 1)
                readChoiceBlock(ref CgArrIndex);
            else
                readValueBlock(ref CgArrIndex);
        }
        string doMsg4TctWindow(CMD src, byte[] content)
        {
            byte[] msg = { 0x26, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            };
            msg[0x01] = (byte)(0x30 + (byte)src);
            byte index;
            for (byte ch = 0; ch < content.Length; ch++)
            {
                index = (byte)(2 * ch + 2);
                if (content[ch] < 0x20)
                {
                    msg[index] = (byte)'%';
                    msg[index + 1] = (byte)(content[ch] + '0');
                }
                else
                {
                    if (content[ch] >= 0x80)
                    {
                        msg[index] = (byte)'&';
                        msg[index + 1] = (byte)(content[ch] - 0x80);
                    }
                    else
                    {
                        msg[index] = (byte)'$';
                        msg[index + 1] = content[ch];
                    }
                }
            }
            // From byte array to string
            return System.Text.Encoding.UTF8.GetString(msg, 0, msg.Length);
        }

        private void fromCAN2UDP()
        {
            byte[] tmpbyte6 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] pattern = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] MFX_LOCID = { 0x00, 0x50, 0x03, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            while (true)
            {
                IPEndPoint remoteIPEndPoint = new IPEndPoint(IPAddress.Any, Cnames.portinCAN);
                try
                {
                    byte[] content = CANServer.Receive(ref remoteIPEndPoint);
                    if (content.Length > 0)
                    {
                        // Das Programm reagiert auf die Erkennung 
                        switch (content[1])
                        {
                            case 0x0F: // ReadConfig_R:
                                {
                                    ConfigStream.getLokName(content[11]);
                                    UpdateProgressMFXBar();
                                }
                                break;
                            case 0x31: // Ping_R:
                                if (receivePINGInfos == true && CANguruArrFilled < 20)
                                {
                                    // CANguru ?
                                    if (content[12] == GFP)
                                    {
                                        for (byte uid = 0; uid < 4; uid++)
                                        {
                                            GFP_UID[uid] = content[5 + uid];
                                        }
                                    }
                                    if (content[12] >= DEVTYPE_BASE && content[12] < DEVTYPE_LastCANguru)
                                    {
                                        bool alreadyKnown = false;
                                        for (byte c = 0; c < CANguruArrFilled; c++)
                                        {
                                            if (content[2] == CANguruPINGArr[c, 2] && content[3] == CANguruPINGArr[c, 3])
                                            {
                                                alreadyKnown = true;
                                                break;
                                            }
                                        }
                                        if (alreadyKnown == false)
                                        {
                                            // UID
                                            for (byte i = 0; i < Cnames.lngFrame; i++)
                                                CANguruPINGArr[CANguruArrFilled, i] = content[i];
                                            // IP-Address auf null
                                            for (byte i = 1; i < 5; i++)
                                                CANguruPINGArr[CANguruArrFilled, Cnames.lngFrame + i] = 0x00;
                                            CANguruArrFilled++;
                                        }
                                    }
                                }
                                break;
                            case 0x3B: // Config
                                       // die Zeilen der Pakete werden alle in das Array
                                       // CANguruArr kopiert
                                for (byte i = 0; i < Cnames.lngFrame; i++)
                                    CANguruArr[CANguruArrLine, i] = content[i];
                                CANguruArrLine++;
                                // 0 ist die Gerätebeschreibung (Paket 0)
                                // dieses Paket ist vollständig, wenn die
                                // Länge der Zeile =6 beträgt
                                if (content[0x04] == 6)
                                {
                                    if (CANguruArrIndex == 0)
                                    {
                                        ChangeMyText(this.TelnetComm, "Decoder angemeldet: " + read1ConfigChannel_DescriptionBlock(ref CANguruArrIndex, ref content));
                                    }
                                    if (CANguruArrIndex > 0)
                                        read1ConfigChannel_ValueBlock(ref CANguruArrIndex);
                                }
                                break;
                            case 0x50: // MfxProc:
                                if (content[4] == 0x01)
                                {
                                    while (myQueue.lngQueue() > 0)
                                    {
                                        pattern = myQueue.eatQueue();
                                        CANClient.Send(pattern, Cnames.lngFrame);
                                    }
                                }
                                break;
                            case 0x51: // MfxProc_R:
                                if (content[5] == 0x01)
                                {
                                    // config stream startet
                                    ConfigStream.startConfigStructmfx(content);
                                }
                                if (content[5] == 0x00)
                                {
                                    // config stream wird beend
                                    ConfigStream.finishConfigStruct();
                                    ConfigStream.incnextLocid();
                                    this.numLocID.Invoke(new MethodInvoker(() => this.numLocID.Value = ConfigStream.getnextLocid()));
                                    MFX_LOCID[5] = ConfigStream.getnextLocid();
                                    ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromCAN, MFX_LOCID));
                                    CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                                    CANClient.Send(MFX_LOCID, Cnames.lngFrame);
                                    this.lokBox.Invoke(new MethodInvoker(() => ConfigStream.editConfigStruct(lokBox)));
                                }
                                break;
                            //#define LoadCS2Data 0x56
                            case 0x56: // ConfigData: compressed & uncompressed
                                if (content[4] == 0x08)
                                {
                                    bool ffound = false;
                                    byte[] arrshortname = new byte[Cnames.shortnameLng2transfer]; // plus 1 Zeichen wg. /0 am Ende
                                    Array.Copy(content, 5, arrshortname, 0, Cnames.shortnameLng2transfer);
                                    string shortname = System.Text.Encoding.Default.GetString(arrshortname);
                                    byte str = 0;
                                    string fileName = "";
                                    if (shortname.Contains("-"))
                                    {
                                        if (cntGleisbild > 0)
                                        {
                                            int page_number;
                                            fileName = @"\gleisbilder/";
                                            page_number = Convert.ToInt32(shortname.Substring(shortname.IndexOf("-") + 1), 10);
                                            if (page_number <= cntGleisbild)
                                            {
                                                if (ConfigStream.page_name[page_number].Length > 0)
                                                {
                                                    fileName += ConfigStream.page_name[page_number] + ".cs2";
                                                    ffound = true;
                                                }
                                            }
                                        }
                                    }
                                    if (!ffound)
                                    {
                                        if (shortname.Contains("+"))
                                        {
                                            if (cntGleisbild > 0)
                                            {
                                                int page_number;
                                                page_number = Convert.ToInt32(shortname.Substring(shortname.IndexOf("+") + 1), 10);
                                                if (page_number <= cntGleisbild)
                                                {
                                                    if (ConfigStream.page_name[page_number].Length > 0)
                                                    {
                                                        fileName = ConfigStream.page_name[page_number] + ".cs2";
                                                        byte[] arrayFN = Encoding.ASCII.GetBytes(fileName);
                                                        CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                                                        CANClient.Send(arrayFN, fileName.Length);
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    if (!ffound)
                                    {
                                        for (str = 0; str < Cnames.cntCS2Files; str++)
                                        {
                                            if (Cnames.cs2Files[str, 0].Contains(shortname))
                                            {
                                                // Langname gefunden, gibt es die zugehörige Datei auch?
                                                fileName = Cnames.cs2Files[str, 1];
                                                ffound = File.Exists(string.Concat(Cnames.path, fileName));
                                                break;
                                            }
                                        }
                                    }
                                    if (ffound == true)
                                    {
                                        ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, content));
                                        byte[] tmpbyte2 = new byte[2];
                                        byte[] tmpbyte4 = new byte[4];
                                        Int16 crc;
                                        // Anzahl der Zeichen übermitteln
                                        if (compress.bcompress)
                                        {
                                            compress compress = new CANguruX.compress();
                                            compress.compressTheData(string.Concat(Cnames.path, fileName));
                                            int compressCnt = compress.getDeflatedSize();
                                            if (compressCnt == 0)
                                            {
                                                // byte 9 und 10 ist der crc-Wert
                                                // crc ist für uncompressed immer null
                                                Array.Copy(tmpbyte6, 0, GETCONFIG_RESPONSE, 5, 6);
                                                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                                                //#define GETCONFIG_RESPONSE LoadCS2Data_R + 0x01     // 0x58
                                                CANClient.Send(GETCONFIG_RESPONSE, GETCONFIG_RESPONSE.Length);
                                                ChangeMyText(this.TelnetComm, "Datei zu groß: " + fileName);
                                                break;
                                            }
                                            compressCnt += 4;
                                            // auf runde 8 auffüllen
                                            if (compressCnt % 8 != 0)
                                            {
                                                compressCnt += 8 - (compressCnt % 8);
                                            }
                                            Array.Resize(ref compress.outBuffer, compressCnt);
                                            tmpbyte4 = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(compress.getDeflatedSize() + 4));
                                            // CRC erzeugen und eintragen
                                            InitialCrcValue initVal = InitialCrcValue.NonZero1;
                                            Crc16Ccitt Crc16Ccitt = new CANguruX.Crc16Ccitt(initVal);
                                            crc = (Int16)Crc16Ccitt.ComputeChecksum(compress.outBuffer);
                                        }
                                        else
                                        {
                                            if (!emptyLokListIsGenerated)
                                            {
                                                ConfigStream.generateEmptyLokList();
                                                emptyLokListIsGenerated = true;
                                            }
                                            ConfigStream.setbufferIndex(0);
                                            ConfigStream.readLocomotive(Cnames.path, fileName);
                                            tmpbyte4 = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(ConfigStream.getbufferIndex()));
                                            crc = 0;
                                        }
                                        crc = IPAddress.HostToNetworkOrder(crc);
                                        tmpbyte2 = BitConverter.GetBytes(crc);
                                        // byte 5 bis 8 ist die Anzahl der zu übertragenden bytes
                                        Array.Copy(tmpbyte4, 0, GETCONFIG_RESPONSE, 5, 4);
                                        // byte 9 und 10 ist der crc-Wert
                                        // crc ist für uncompressed immer null
                                        Array.Copy(tmpbyte2, 0, GETCONFIG_RESPONSE, 9, 2);
                                        //                                        ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, GETCONFIG_RESPONSE));
                                        CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                                        //#define GETCONFIG_RESPONSE LoadCS2Data_R + 0x01     // 0x58
                                        CANClient.Send(GETCONFIG_RESPONSE, GETCONFIG_RESPONSE.Length);
                                    }
                                    else
                                    {
                                        // byte 9 und 10 ist der crc-Wert
                                        // crc ist für uncompressed immer null
                                        Array.Copy(tmpbyte6, 0, GETCONFIG_RESPONSE, 5, 6);
                                        CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                                        //#define GETCONFIG_RESPONSE LoadCS2Data_R + 0x01     // 0x58
                                        CANClient.Send(GETCONFIG_RESPONSE, GETCONFIG_RESPONSE.Length);
                                        ChangeMyText(this.TelnetComm, "Datei nicht gefunden: " + fileName);
                                    }
                                }
                                break;
                            //#define LoadCS2Data_R LoadCS2Data + 0x01            // 0x57
                            case 0x57: // ConfigData_R: compressed & uncompressed
                                if (content[4] == 0x08)
                                {
                                    if (compress.bcompress)
                                    {
                                        compress.sendBufferCompressed(content, CANClient);
                                    }
                                    else
                                    {
                                        ConfigStream.sendBufferUncompressed(content, CANClient);
                                    }
                                }
                                break;
                            //#define DoCompress GETCONFIG_RESPONSE + 0x02        // 0x5A
                            case 0x5A: // DoCompress
                                compress.bcompress = true;
                                break;
                            //#define DoNotCompress DoCompress + 0x01             // 0x5B
                            case 0x5B: // DoNotCompress
                                compress.bcompress = false;
                                break;
                            case 0x89:
                                gettingConnectionTimer.Enabled = false;
                                Cnames.IP_CAN = remoteIPEndPoint.Address.ToString();
                                this.tbConnectAdr.Invoke(new MethodInvoker(() => this.tbConnectAdr.Text = Cnames.IP_CAN));
                                connectThread = new Thread(this.connectServer);
                                connectThread.Start();
                                is_connected ^= true;
                                this.buttonConnect.Invoke(new MethodInvoker(() => this.buttonConnect.Enabled = false));
                                this.btnVolt.Invoke(new MethodInvoker(() => this.btnVolt.Enabled = true));
                                this.MFX.Invoke(new MethodInvoker(() => this.MFX.Enabled = true));
                                this.Configuration.Invoke(new MethodInvoker(() => this.Configuration.Enabled = true));
                                this.lokBox.Invoke(new MethodInvoker(() => ConfigStream.editConfigStruct(lokBox)));
                                Seta1milliTimer();
                                break;
                            case 0x65:
                                for (byte c = 0; c < CANguruArrFilled; c++)
                                {
                                    // 00 65 UU UU 08 IP IP IP IP 00 00 00 00
                                    if (content[2] == CANguruPINGArr[c, 2] && content[3] == CANguruPINGArr[c, 3])
                                    {
                                        // IP-Address auf null
                                        for (byte i = 1; i < 5; i++)
                                            CANguruPINGArr[c, Cnames.lngFrame + i] = content[4 + i];
                                        break;
                                    }
                                }
                                break;
                        }
                    }
                    // alle Rückmeldungen vom CAN oder Client
                    /*                   byte cmd = content[0x01];
                                       if (((cmd & 0x01) == 0x01) && (cmd != 0x89))
                                           cmd = 0x00;
                                       else
                                           cmd = content[0x01];
                                       switch (cmd)
                                       {
                                           case 0x00:
                                               ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromCAN, content));
                                               break;
                                           case 0x41:
                                           case 0x56:
                                           case 0x57:
                                           case 0x58:
                                           case 0x89:
                                               break;
                                           default:
                                               ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.toGW, content));
                                               break;
                                       }*/
                    ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.toGW, content));
                }
                catch (Exception e)
                {
                    MessageBox.Show("Threat-Exception: " + e.Message);
                }
            }
        }

        private void mfxLoks_Click(object sender, EventArgs e)
        {
            // Gleisstrom abschalten
            byte[] MFX_STOP = { 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, MFX_STOP));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(MFX_STOP, Cnames.lngFrame);
            elapsedsec = 0;
            Seta1secTimer();
            setProgressMFXBar((int)(14));
        }

        private void After1sec(Object source, ElapsedEventArgs e)
        {
            byte[] MFX_COUNTER = { 0x00, 0x00, 0x03, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00 };
            byte[] MFX_GO = { 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
            elapsedsec++;
            UpdateProgressMFXBar();
            switch (elapsedsec)
            {
                case 1:
                    // Neuanmeldezähler erhöhen
                    ConfigStream.incCounter();
                    this.numCounter.Invoke(new MethodInvoker(() => this.numCounter.Value = ConfigStream.getCounter()));
                    for (byte uid = 0; uid < 4; uid++)
                    {
                        MFX_COUNTER[5 + uid] = GFP_UID[uid];
                    }
                    MFX_COUNTER[11] = ConfigStream.getCounter();
                    ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, MFX_COUNTER));
                    CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                    CANClient.Send(MFX_COUNTER, Cnames.lngFrame);
                    break;
                case 10:
                    // Gleisstrom anschalten
                    ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, MFX_GO));
                    CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                    CANClient.Send(MFX_GO, Cnames.lngFrame);
                    break;
                case 15:
                    a1secTimer.Enabled = false;
                    break;
            }
        }

        private void Seta1secTimer()
        {
            // Create a timer with a two second interval.
            a1secTimer = new System.Timers.Timer(1000);
            // Hook up the Elapsed event for the timer. 
            a1secTimer.Elapsed += After1sec;
            a1secTimer.AutoReset = true;
            a1secTimer.Enabled = true;
        }

        private void genLokListmfx_Click(object sender, EventArgs e)
        {
            byte[] MFX_LOCLIST = { 0x00, 0x51, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            // erzeuge neue Liste
            ConfigStream.generateEmptyLokList();
            ConfigStream.generateLokListwithListbox(lokBox);
            // rege CANguru an, neue Liste zu laden
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, MFX_LOCLIST));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(MFX_LOCLIST, Cnames.lngFrame);
        }

        private void getLok_Click(object sender, EventArgs e)
        {
            string name = txtBoxLokName.Text;
            byte adr = (byte)numLokAdress.Value;
            byte type = (byte)lstBoxDecoderType.SelectedIndex;
            ConfigStream.fillConfigStruct(name, adr, type);
            ConfigStream.finishConfigStruct();
        }

        void findLoks_Click(object sender, EventArgs e)
        {
            ConfigStream.editConfigStruct(lokBox);
        }

        private void delLok_Click(object sender, EventArgs e)
        {
            ConfigStream.delConfigStruct(lokBox);
        }

        private void numCounter_ValueChanged(object sender, EventArgs e)
        {
            ConfigStream.setCounter((byte)(numCounter.Value));
        }

        private void numLocID_ValueChanged(object sender, EventArgs e)
        {
            ConfigStream.setnextLocid((byte)(numLocID.Value));
        }

        private void beenden_Click(object sender, EventArgs e)
        {
            string message = "Wollen Sie wirklich beenden?";
            string caption = "Beenden";
            MessageBoxButtons buttons = MessageBoxButtons.YesNo;
            DialogResult result = MessageBox.Show(this, message, caption, buttons);

            if (result == DialogResult.Yes)
            {
                // Aufräumen
                ini.RemoveAllSections();
                ini.AddSection("IP-address").AddKey("IPCAN").Value = Cnames.IP_CAN;
                ConfigStream.setCounter((byte)(numCounter.Value));
                byte c = ConfigStream.getCounter();
                ini.AddSection("Neuanmeldezaehler").AddKey("Counter").Value = c.ToString();
                ConfigStream.setnextLocid((byte)(numLocID.Value));
                byte n = ConfigStream.getnextLocid();
                ini.AddSection("Lok-Adresse").AddKey("LocID").Value = n.ToString();
                string v = "0";
                if (verbose)
                    v = "1";
                ini.AddSection("Verbose").AddKey("verbose").Value = v;
                //Save the INI
                ini.Save(string.Concat(Cnames.path, Cnames.ininame));
                voltStop();
                restartTheBridge();
                Environment.Exit(0x1);
            }
        }

        public static String GetMyText(Control ctrl)
        {
            if (ctrl.InvokeRequired)
            {
                var del = new GetMyTextDelegate(GetMyText);
                ctrl.Invoke(del, ctrl);
                return "";
            }
            else
            {
                return ctrl.Text;
            }
        }

        static byte decodeChar(byte x, byte y)
        {
            byte cmd = 0;
            switch (x)
            {
                case 0x24: // $
                    cmd = y;
                    break;
                case 0x25: // %
                    cmd = (byte)(y - 0x30);
                    break;
                case 0x26: // &
                    cmd = (byte)(y + 0x80);
                    break;
            }

            return cmd;
        }

        static string getDataText(string text)
        {
            string res = "";
            byte[] ch = Encoding.ASCII.GetBytes(text);
            byte[] bytes = new byte[8];
            int j = 0;
            for (int i = 12; i < 12 + 2 * 8; i = i + 2)
            {
                bytes[j] = decodeChar(ch[i], ch[i + 1]);
                j++;
            }
            char[] chars = Encoding.UTF8.GetChars(bytes);
            for (int i = 0; i < chars.Length; i++)
            {
                if (chars[i] < ' ' || chars[i] > 'z')
                    res += " ";
                else
                    res += chars[i];
            }
            return res;
        }

        public static void ChangeMyText(TextBox ctrl, string text)
        {
            // toCAN, toClnt, toUDP, toTCP, fromCAN, fromClnt, fromUDP, fromTCP, 
            // fromUDP2CAN, fromTCP2CAN, fromTCP2Clnt, fromCAN2UDP, fromCAN2TCP, fromGW2Clnt, fromGW2CAN,
            // toGW, fromGW
            String[] source_dest = {
            //   toCAN
                "    >CAN> 0x",
            //   toClnt
                "    >Clnt>0x",
            //   toUDP
                "    >UDP> 0x",
            //   toTCP
                "    >TCP> 0x",
            //   fromCAN
                ">CAN>     0x",
            //   fromClnt
                ">Clnt>    0x",
            //   fromUDP
                ">UDP>     0x",
            //   fromTCP
                ">TCP>     0x",
            //   fromUDP2CAN
                ">UDP>CAN> 0x",
            //   fromTCP2CAN
                ">TCP>CAN> 0x",
            //   fromTCP2Clnt
                ">TCP>Clnt>0x",
            //   fromCAN2UDP
                ">CAN>UDP> 0x",
            //   fromCAN2TCP
                ">CAN>TCP> 0x",
            //   fromGW2Clnt
                ">GW>Clnt> 0x",
            //   fromGW2CAN
                ">GW>CAN > 0x",
            //   toGW
                "    >G_W> 0x",
            //   fromGW
                ">G_W>     0x"
            };
            if (!is_connected)
                return;
            if (ctrl.InvokeRequired)
            {
                var del = new ChangeMyTextDelegate(ChangeMyText);
                ctrl.Invoke(del, ctrl, text);
            }
            else
            {
                if (text.Length > 1)
                {
                    if (text.StartsWith("&"))
                    {
                        byte[] ch = Encoding.ASCII.GetBytes(text);
                        byte[] arr = new byte[13];
                        if (ch.Length < 28)
                        {
                            for (int i = 0; i < 28; i++)
                                text += "$?";
                            ch = Encoding.ASCII.GetBytes(text);
                        }
                        byte currCMD = decodeChar(ch[4], ch[5]);
                        // keine Wiederholungen
                        // außer Geschwindigkeit
                        if ((lastCMD == 0x08) && (currCMD == 0x08) ||
                        // außer Rückmeldung
                            (lastCMD == 0x23) && (currCMD == 0x23))
                            lastCMD = 0x00;
                        bool repeat = lastCMD == currCMD;
                        lastCMD = currCMD;
                        if (repeat)
                        {
                            return;
                        }
                        if (!verbose)
                        {
                            // data 0 - 12/13
                            // data 1 - 14/15
                            // data 2 - 16/17
                            // data 3 - 18/19
                            // data 4 - 20/21
                            // data 5 - 22/23
                            // data 6 - 24/25
                            // data 7 - 26/27
                            // der Hashwert zur Unterscheidung aus der
                            // aktuellen Zeile Position 2 und 3
                            string hash = " - " + String.Format("{0:X02}", decodeChar(ch[6], ch[7]));
                            hash += String.Format("{0:X02}", decodeChar(ch[8], ch[9]));
                            switch (currCMD)
                            {
                                // Lok_Speed
                                case 0x08:
                                    string speed16 = String.Format("{0:X02}", decodeChar(ch[20], ch[21]));
                                    speed16 += String.Format("{0:X02}", decodeChar(ch[22], ch[23]));
                                    uint numSpeed = uint.Parse(speed16, System.Globalization.NumberStyles.AllowHexSpecifier);
                                    byte[] intValsSpeed = BitConverter.GetBytes(numSpeed);
                                    int speed10 = BitConverter.ToInt32(intValsSpeed, 0);
                                    ctrl.AppendText("Neue Geschwindigkeit: " + speed10);
                                    ctrl.AppendText(Environment.NewLine);
                                    return;
                                // Lok_Direction
                                case 0x0A:
                                    ctrl.AppendText("Neue Richtung" + hash);
                                    ctrl.AppendText(Environment.NewLine);
                                    return;
                                // Gleisbesetztmelder / Kontaktstelle
                                case 0x23:
                                    string contact16 = String.Format("{0:X02}", decodeChar(ch[18], ch[19]));
                                    uint numContact = uint.Parse(contact16, System.Globalization.NumberStyles.AllowHexSpecifier);
                                    byte[] intValsContact = BitConverter.GetBytes(numContact);
                                    int contact10 = BitConverter.ToInt32(intValsContact, 0);
                                    string status = " - Status: ";
                                    if (decodeChar(ch[22], ch[23]) == 1)
                                        status += "AN";
                                    else
                                        status += "AUS";
                                    ctrl.AppendText("Kontakt / Gleis: " + contact10 + status);
                                    ctrl.AppendText(Environment.NewLine);
                                    return;
                                // die folgenden Meldungen werden kommentiert angezeigt
                                case 0x30:
                                    ctrl.AppendText("PING angefragt" + hash);
                                    ctrl.AppendText(Environment.NewLine);
                                    return;
                                case 0x31:
                                    ctrl.AppendText("PING Rückmeldung" + hash);
                                    ctrl.AppendText(Environment.NewLine);
                                    return;
                                // ConfigDataCompressed_R
                                case 0x41:
                                // LoadCS2Data
                                case 0x56:
                                    ctrl.AppendText("Datei angefordert - " + getDataText(text));
                                    ctrl.AppendText(Environment.NewLine);
                                    return;
                                // die folgenden Meldungen werden unterdrückt
                                // System
                                case 0x00:
                                case 0x01:
                                // Lok_Speed_R
                                case 0x09:
                                // Lok_Direction_R
                                case 0x0B:
                                // MAGIC
                                case 0x36:
                                // CONFIG_Status
                                case 0x3A:
                                case 0x3B:
                                // DoCompress
                                // LoadCS2Data Rückmeldung
                                case 0x57:
                                case 0x5A:
                                // DoNotCompress
                                case 0x5B:
                                // send IP
                                case 0x64:
                                case 0x65:
                                // first feedback: here i am
                                case 0x89:
                                    return;
                            }
                        }
                        text = source_dest[ch[1] - '0'];
                        int chPtr;
                        for (int c = 0; c < 13; c++)
                        {
                            chPtr = 2 * c + 2;
                            switch (ch[chPtr])
                            {
                                case 0x24: // $
                                    arr[c] = (byte)ch[chPtr + 1];
                                    break;
                                case 0x25: // %
                                    arr[c] = (byte)(ch[chPtr + 1] - 0x30);
                                    break;
                                case 0x26: // &
                                    arr[c] = (byte)(ch[chPtr + 1] + 0x80);
                                    break;
                            }
                        }
                        StringBuilder builder = new StringBuilder();
                        // Merge all bytes into a string of bytes  
                        builder.Append(arr[0].ToString("X2"));
                        builder.Append("(");
                        builder.Append(arr[1].ToString("X2"));
                        builder.Append(")");
                        for (int i = 2; i < 4; i++)
                        {
                            builder.Append(arr[i].ToString("X2"));
                        }
                        if ((arr[1] & 0x01) == 0x01)
                            builder.Append(" R ");
                        else
                            builder.Append("   ");
                        builder.Append("[");
                        builder.Append(arr[4].ToString("X2"));
                        builder.Append("]");
                        byte[] bytes = new byte[8];
                        for (int i = 5; i < arr.Length; i++)
                        {
                            builder.Append(" ");
                            builder.Append(arr[i].ToString("X2"));
                            bytes[i - 5] = arr[i];
                        }
                        builder.Append(" ");
                        char[] chars = Encoding.UTF8.GetChars(bytes);
                        for (int i = 0; i < chars.Length; i++)
                        {
                            if (chars[i] < ' ' || chars[i] > 'z')
                                builder.Append(".");
                            else
                                builder.Append(chars[i]);
                        }
                        text += builder.ToString();
                    }
                    if (text.StartsWith("!"))
                    {
                        text = text.Substring(1);
                    }
                    if (text.StartsWith("%"))
                    {
                        return;
                    }
                }
                ctrl.AppendText(text);
                ctrl.AppendText(Environment.NewLine);
            }
        }

        private void connectServer()
        {
            try
            {
                this.Client = new TcpClient(Cnames.IP_CAN, Cnames.port);
                stream = this.Client.GetStream();
                reader = new StreamReader(stream);
                var line = "";
                while ((line = reader.ReadLine()) != "END")
                {

                    ChangeMyText(this.TelnetComm, line);
                }
            }

            catch (Exception e)
            {
                MessageBox.Show("ConnectException: " + e.Message);
            }
            finally
            {
            }
        }

        void getConfigData(byte canguru, byte index)
        {
            byte[] GET_IP = { 0x00, 0x64, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] GETCONFIG = { 0x00, 0x3A, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            if (canguru >= CANguruArrFilled)
            {
                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                CANClient.Send(GET_IP, Cnames.lngFrame);
                return;
            }
            // UID eintragen
            for (byte i = 5; i < 9; i++)
                GETCONFIG[i] = CANguruPINGArr[canguru, i];
            // Paketnummer
            GETCONFIG[9] = index;
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, GETCONFIG));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(GETCONFIG, Cnames.lngFrame);
        }

        private void After1milli(Object source, ElapsedEventArgs e)
        {
            elapsedmillis++;
            switch (elapsedmillis)
            {
                case 1:
                    receivePINGInfos = true;
                    //[maxCANgurus] [maxConfigLines+1] [Cnames.lngFrame]
                    for (byte y = 0; y < Cnames.maxConfigLines; y++)
                        for (byte z = 0; z < Cnames.lngFrame; z++)
                        {
                            CANguruArr[y, z] = 0;
                        }
                    for (byte y = 0; y < Cnames.maxCANgurus; y++)
                        for (byte z = 0; z < Cnames.lngFrame + 1; z++)
                        {
                            CANguruPINGArr[y, z] = 0;
                        }
                    CANguruArrFilled = 0;
                    CANguruArrWorked = 0;
                    CANguruArrLine = 0;
                    CANguruArrIndex = 0;
                    break;
                case 200:
                    a1milliTimer.Enabled = false;
                    receivePINGInfos = false;
                    getConfigData(CANguruArrWorked, CANguruArrIndex);
                    break;
            }
        }

        private void Seta1milliTimer()
        {
            // Create a timer with a 1 miili interval.
            a1milliTimer = new System.Timers.Timer(1);
            // Hook up the Elapsed event for the timer. 
            a1milliTimer.Elapsed += After1milli;
            a1milliTimer.AutoReset = true;
            a1milliTimer.Enabled = true;
            elapsedmillis = 0;
        }

        bool checkFiles()
        {
            string message = "Die Datei fehlt: ";
            string caption = "Fehler";
            for (byte f = 0; f < Cnames.cntCS2Files; f++)
            {
                string m_filename = string.Concat(Cnames.path, Cnames.cs2Files[f, 1]);
                if (Cnames.cs2Files[f, 2] == "1")
                    if (!File.Exists(m_filename))
                    {
                        message += m_filename;
                        MessageBoxButtons buttons = MessageBoxButtons.OK;
                        MessageBox.Show(this, message, caption, buttons);
                        return false;
                    }
            }
            return true;
        }

        private void gettingConnection(Object source, ElapsedEventArgs e)
        {
            byte[] M_SEND_IP = { 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            Cnames.IP_CAN = "255.255.255.255";
            try
            {
                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            }
            catch (Exception exception)
            {
                MessageBox.Show("Scan-Exception: " + exception.Message);
            }
            CANClient.Send(M_SEND_IP, Cnames.lngFrame);
            elapsedmillis4Connection++;
            if (elapsedmillis4Connection > 250)
            {
                this.buttonConnect.Invoke(new MethodInvoker(() => this.buttonConnect.Enabled = true));
                gettingConnectionTimer.Enabled = false;
            }
        }

        private void SetgettingConnectionTimer()
        {
            // Create a timer with a 20 milli interval.
            gettingConnectionTimer = new System.Timers.Timer(20);
            // Hook up the Elapsed event for the timer. 
            gettingConnectionTimer.Elapsed += gettingConnection;
            gettingConnectionTimer.AutoReset = true;
            gettingConnectionTimer.Enabled = true;
            elapsedmillis4Connection = 0;
            this.buttonConnect.Invoke(new MethodInvoker(() => this.buttonConnect.Enabled = false));
        }

        // fragt die selektierte Host- oder Gatewayadresse ab und sendet das
        // CAN-Frame M_SEND_IP in dieses Netzwerk; die CANguru-Bridge meldet sich dann anschließeend;
        // diese Antwort wird in fromCAN2UDP() unter 0x89 empfangen und ausgewertet
        public void onConnectClick(object sender, EventArgs e)
        {
            cntGleisbild = ConfigStream.read_track_file();
            if (!checkFiles())
                return;
            SetgettingConnectionTimer();
        }

        private void setProgressMFXBar(int max)
        {
            progressBarMfx.Visible = true;
            // Set Minimum to 1 to represent the first file being copied.
            progressBarMfx.Minimum = 1;
            // Set Maximum to the total number of files to copy.
            progressBarMfx.Maximum = max;
            // Set the initial value of the ProgressBar.
            progressBarMfx.Value = 1;
            // Set the Step property to a value of 1 to represent each file being copied.
            progressBarMfx.Step = 1;
        }

        // your form's methods and properties here, as usual
        private void UpdateProgressPingBar()
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (this.progressBarPing.InvokeRequired)
            {
                this.Invoke(UpdateProgressPingBarMethod, new object[] { });
            }
            else
            {
                if (progressBarPing.Value >= progressBarPing.Maximum)
                    progressBarPing.Visible = false;
                else
                    progressBarPing.PerformStep();
            }
        }

        // your form's methods and properties here, as usual
        private void UpdateProgressMFXBar()
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (this.progressBarMfx.InvokeRequired)
            {
                this.Invoke(UpdateProgressMFXBarMethod, new object[] { });
            }
            else
            {
                if (progressBarMfx.Value >= progressBarMfx.Maximum)
                    progressBarMfx.Visible = false;
                else
                    progressBarMfx.PerformStep();
            }
        }

        private void voltStop()
        {
            byte[] VOLT_STOP = { 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, VOLT_STOP));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(VOLT_STOP, Cnames.lngFrame);
            Voltage = false;
        }
        static void TCPConnect(String message)
        {
            try
            {
                // Create a TcpClient.
                // Note, for this client to work you need to have a TcpServer
                // connected to the same address as specified by the server, port
                // combination.
                Int32 port = 8080; // Cnames.portoutCAN-2;
                TcpClient client = new TcpClient(Cnames.IP_CAN, port);

                // Translate the passed message into ASCII and store it as a Byte array.
                Byte[] data = System.Text.Encoding.ASCII.GetBytes(message);

                // Get a client stream for reading and writing.
                //  Stream stream = client.GetStream();

                NetworkStream stream = client.GetStream();

                // Send the message to the connected TcpServer.
                stream.Write(data, 0, data.Length);

                Console.WriteLine("Sent: {0}", message);

                // Receive the TcpServer.response.

                // Buffer to store the response bytes.
                data = new Byte[256];

                // String to store the response ASCII representation.
                String responseData = String.Empty;

                // Read the first batch of the TcpServer response bytes.
                Int32 bytes = stream.Read(data, 0, data.Length);
                responseData = System.Text.Encoding.ASCII.GetString(data, 0, bytes);
                Console.WriteLine("Received: {0}", responseData);

                // Close everything.
                stream.Close();
                client.Close();
            }
            catch (ArgumentNullException e)
            {
                Console.WriteLine("ArgumentNullException: {0}", e);
            }
            catch (SocketException e)
            {
                Console.WriteLine("SocketException: {0}", e);
            }

            Console.WriteLine("\n Press Enter to continue...");
            Console.Read();
        }

        private void btnVolt_Click(object sender, EventArgs e)
        {
            Button btn = (Button)sender;
            byte[] VOLT_GO = { 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };

            if (Voltage)
            {
                voltStop();
                btn.Text = "Gleisspannung EIN";
            }
            else
            {
                ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, VOLT_GO));
                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                CANClient.Send(VOLT_GO, Cnames.lngFrame);
                btn.Text = "Gleisspannung AUS";
                Voltage = true;
            }
        }

        private void showConfigData(byte arrWorked, byte arrIndex)
        {
            byte[] SET_CONFIG = { 0x00, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00 };
            if (is_connected == false)
                return;
            CANguruArrWorked = arrWorked;
            CANguruArrLine = 0;
            CANguruArrIndex = arrIndex;
            // UID eintragen
            for (byte i = 5; i < 9; i++)
                SET_CONFIG[i] = CANguruPINGArr[lastSelectedItem, i];
            // alte Controls löschen
            byte c = configControls.cntControls;
            for (byte cc = 0; cc < c; cc++)
            {
                // Kanal setzen
                SET_CONFIG[10] = cc;
                SET_CONFIG[10]++;
                // Wert setzen
                SET_CONFIG[11] = 0;
                if (CANguruConfigArr[cc].indicatorNumericUpDown != null)
                {
                    decimal val = CANguruConfigArr[cc].indicatorNumericUpDown.Value;
                    // Get the bytes of the decimal
                    byte[] valBytes = Encoding.ASCII.GetBytes(val.ToString());
                    switch (valBytes.Length)
                    {
                        case 1:
                            int b10 = valBytes[0] - 0x30;
                            SET_CONFIG[12] = (byte)(b10);
                            break;
                        case 2:
                            int b20 = valBytes[1] - 0x30;
                            int b21 = valBytes[0] - 0x30;
                            SET_CONFIG[12] = (byte)(b21 * 10 + b20);
                            break;
                        case 3:
                            int b30 = valBytes[2] - 0x30;
                            int b31 = valBytes[1] - 0x30;
                            int b32 = valBytes[0] - 0x30;
                            int v3 = b32 * 100 + b31 * 10 + b30;
                            SET_CONFIG[11] = (byte)(v3 >> 8);
                            SET_CONFIG[12] = (byte)(v3 & 0x00FF);
                            break;
                        case 4:
                            int b40 = valBytes[3] - 0x30;
                            int b41 = valBytes[2] - 0x30;
                            int b42 = valBytes[1] - 0x30;
                            int b43 = valBytes[0] - 0x30;
                            int v4 = b43 * 1000 + b42 * 100 + b41 * 10 + b40;
                            SET_CONFIG[11] = (byte)(v4 >> 8);
                            SET_CONFIG[12] = (byte)(v4 & 0x00FF);
                            break;
                        default:
                            SET_CONFIG[11] = 0;
                            SET_CONFIG[12] = 0;
                            break;
                    }
                }
                if (CANguruConfigArr[cc].indicatorListBox != null)
                {
                    SET_CONFIG[11] = 0;
                    SET_CONFIG[12] = (byte)CANguruConfigArr[cc].indicatorListBox.SelectedIndex;
                }
                // den (neuen) Wert senden
                ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, SET_CONFIG));
                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                CANClient.Send(SET_CONFIG, Cnames.lngFrame);
                Task.Delay(50).Wait();
                // Löschen der Controls
                if (Configuration.Controls.Contains(configControls.controlLabel[cc]))
                    Configuration.Controls.Remove(configControls.controlLabel[cc]);
                if (CANguruConfigArr[cc].indicatorNumericUpDown != null)
                {
                    if (Configuration.Controls.Contains(configControls.controlValue[cc]))
                        Configuration.Controls.Remove(configControls.controlValue[cc]);
                }
                if (CANguruConfigArr[cc].indicatorListBox != null)
                {
                    if (Configuration.Controls.Contains(configControls.controlChoice[cc]))
                        Configuration.Controls.Remove(configControls.controlChoice[cc]);
                }
                if (Configuration.Controls.Contains(configControls.controlUnit[cc]))
                    Configuration.Controls.Remove(configControls.controlUnit[cc]);
            }
            // Platz ür neue Controls
            byte maxIndex = CANguruPINGArr[CANguruArrWorked, Cnames.lngFrame];
            CANguruConfigArr = new configStruct[maxIndex];
            configControls.cntControls = maxIndex;
            configControls.controlLabel = new Control[maxIndex];
            configControls.controlUnit = new Control[maxIndex];
            configControls.controlValue = new Control[maxIndex];
            configControls.controlChoice = new Control[maxIndex];
            getConfigData(CANguruArrWorked, CANguruArrIndex);
        }
        private void CANElemente_SelectedIndexChanged(object sender, EventArgs e)
        {
            int curItem = CANElemente.SelectedIndex;
            if (curItem >= 0)
            {
                showConfigData((byte)curItem, 1);
                string IPAddress;
                IPAddress = String.Format("{0:D03}", CANguruPINGArr[curItem, 0x0E]) + ".";
                IPAddress += String.Format("{0:D03}", CANguruPINGArr[curItem, 0x0F]) + ".";
                IPAddress += String.Format("{0:D03}", CANguruPINGArr[curItem, 0x10]) + ".";
                IPAddress += String.Format("{0:D03}", CANguruPINGArr[curItem, 0x11]);
                this.deviceIP.Invoke(new MethodInvoker(() => this.deviceIP.Text = IPAddress));
            }
        }

        private void TabControl1_SelectedIndexChanged(object sender, EventArgs e)
        {
            // initialize ota
            if (this.tabControl1.SelectedIndex == 2)
            {
                configControls.cntControls = 0;
                int cnt = CANElemente.Items.Count;
                if (cnt > 0)
                {
                    CANElemente.SetSelected(0, true);
                }
            }
        }

        private void otaBtn_Click(object sender, EventArgs e)
        {
            // initialize ota
            byte[] OTA_START = { 0x00, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 };

            if (is_connected == false)
                return;
            // UID eintragen
            for (byte i = 5; i < 9; i++)
                OTA_START[i] = CANguruPINGArr[lastSelectedItem, i];
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, OTA_START));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(OTA_START, Cnames.lngFrame);
        }

        private void resetButton_Click(object sender, EventArgs e)
        {
            // initialize ota
            byte[] RESET_START = { 0x00, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00 };
            string message = "Wollen Sie wirklich alle Werte zurücksetzen??";
            string caption = "Reset";
            MessageBoxButtons buttons = MessageBoxButtons.YesNo;
            DialogResult result = MessageBox.Show(this, message, caption, buttons);

            if (is_connected == false)
                return;
            if (result == DialogResult.Yes)
            {
                // Aufräumen
                // UID eintragen
                for (byte i = 5; i < 9; i++)
                    RESET_START[i] = CANguruPINGArr[lastSelectedItem, i];
                ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, RESET_START));
                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                CANClient.Send(RESET_START, Cnames.lngFrame);
            }
        }

        private void restartTheBridge()
        {
            byte[] RESTART_BRIDGE = { 0x00, 0x62, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, RESTART_BRIDGE));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(RESTART_BRIDGE, Cnames.lngFrame);
            // Alles löschen
            is_connected = false;
            TelnetComm.Clear();
            for (int n = CANElemente.Items.Count - 1; n >= 0; --n)
            {
                CANElemente.Items.RemoveAt(n);
            }
            myQueue.resetQueue();
        }

        private void btnVerbose_Click(object sender, EventArgs e)
        {
            if (verbose)
            {
                btnVerbose.Text = "Verbose";
                verbose = false;
            }
            else
            {
                btnVerbose.Text = "Not verbose";
                verbose = true;
            }
        }
    }
}
