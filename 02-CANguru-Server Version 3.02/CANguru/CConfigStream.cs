using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Windows.Forms;

namespace CANguruX
{
    class CConfigStream
    {
        Cnames names;
        Cutils utils;
        static byte[] inBuffer;
        static byte[] mfxBuffer;
        static byte[] outBuffer = new byte[0x8000];
        static UInt16 bufferIndex = 0;

        struct configstruct
        {
            public byte[] session;
            public byte[] locid;
            public byte[] typ;
            public byte[] mfxuid;
            public byte[] lokname;
            public byte nameIndex;
        }
        static configstruct[] lokconfig;
        byte Counter;
        byte nextLocid;

        public const byte minCounter = 0x00;
        public const byte maxCounter = 0xF0;
        public const byte minLocID = 0x01;
        public const byte maxLocID = 0xF0;

        // gleisbild
        const int MAXLINE = 1024;
        public string[] page_name = new string[MAXLINE];

        // Public constructor
        public CConfigStream(Cnames n)
        {
            utils = new Cutils();
            names = n;
        }

        public UInt16 getbufferIndex()
        {
            return bufferIndex;
        }

        public void setbufferIndex(UInt16 b)
        {
            bufferIndex = b;
        }
        // Neuanmeldezähler
        public byte getMinCounter()
        {
            return minCounter;
        }

        public byte getMaxCounter()
        {
            return maxCounter;
        }

        public byte getCounter()
        {
            return Counter;
        }

        public void setCounter(byte c)
        {
            Counter = c;
        }

        public void incCounter()
        {
            Counter++;
            if (Counter > maxCounter)
                Counter = minCounter;
        }
        // Schienenadresse
        public byte getMinLocID()
        {
            return minLocID;
        }

        public byte getMaxLocID()
        {
            return maxLocID;
        }

        public byte getnextLocid()
        {
            return nextLocid;
        }

        public void setnextLocid(byte n)
        {
            nextLocid = n;
        }

        public void incnextLocid()
        {
            nextLocid++;
            if (nextLocid > maxLocID)
                nextLocid = minLocID;
        }

        public void readLocomotive(string p, string cname)
        {
            string cs2 = string.Concat(p, cname);
            bool fexists = File.Exists(cs2);
            if (fexists)
            {
                FileStream fs = new FileStream(cs2, FileMode.Open);
                // Read bytes
                bufferIndex = (ushort)fs.Length;
                inBuffer = new byte[bufferIndex];
                fs.Read(inBuffer, 0, bufferIndex);
                fs.Close();
            }
            else
                bufferIndex = 0;
        }

        public int read_track_file()
        {
            int res = 0;
            bool page = false;
            int id = 0;
            int counter = 0;
            string line;
            string fName = "";

            string f = string.Concat(Cnames.path, Cnames.glsbild);

            try
            {
                if (!File.Exists(f))
                {
                    return 0;
                }
                using (StreamReader cs2 = new StreamReader(new FileStream(f, FileMode.Open)))
                {
                    while (cs2.Peek() >= 0)
                    {
                        line = cs2.ReadLine();
                        if (line == "seite")
                            page = true;
                        else if (line.Contains(".id"))
                            id = Convert.ToInt32(line.Substring(line.IndexOf("=") + 1), 10);
                        else if (line.Contains(".name"))
                        {
                            fName = line.Substring(line.IndexOf("=") + 1);
                            if (page)
                            {
                                page_name[id] = fName;
                                if (page_name[id] == "")
                                    return res;
                                counter++;
                            }
                        }
                    }
                    return counter;
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("The process failed: {0}", e.ToString());
            }
            return res;
        }

        private static IEnumerable<int> PatternAt(byte[] source, byte[] pattern, int start)
        {
            for (int i = start; i < source.Length; i++)
            {
                if (source.Skip(i).Take(pattern.Length).SequenceEqual(pattern))
                {
                    yield return i;
                }
            }
        }

        private static Array RemoveAt(Array source, int index, int tochange)
        {
            if (tochange == 0)
                return source;
            else
            {
                Array dest = Array.CreateInstance(source.GetType().GetElementType(), source.Length + tochange);
                Array.Copy(source, 0, dest, 0, index);
                if (tochange > 0)
                    Array.Copy(source, index, dest, index + tochange, source.Length - index);
                else
                    Array.Copy(source, index - tochange, dest, index, source.Length - index + tochange);

                return dest;
            }
        }

        private int insertToken(byte patternOffset, byte[] token, int start, byte lng)
        {
            byte[] pattern;
            int deleteMore = 0;
            if (patternOffset < 10)
            {
                pattern = new byte[] { 0x4D, 0x4D, 0x4D, 0x4D };
                for (byte o = 0; o < 4; o++)
                    pattern[o] += patternOffset;
            }
            else
            {
                switch (patternOffset)
                {
                    case 10:
                        //                               .     s     i     d     =     0     x     R     R     R     R
                        pattern = new byte[] { 0x20, 0x2E, 0x73, 0x69, 0x64, 0x3D, 0x30, 0x78, 0x52, 0x52, 0x52, 0x52 };
                        deleteMore = 2;
                        break;
                    case 11:
                        //                   { ' ',  '.',  'm',  'f',  'x',  'u',  'i', 'd',   '=', '0',   'x',  'S',  'S',  'S',  'S' };
                        pattern = new byte[] { 0x20, 0x2E, 0x6D, 0x66, 0x78, 0x75, 0x69, 0x64, 0x3D, 0x30, 0x78, 0x53, 0x53, 0x53, 0x53 };
                        deleteMore = 2;
                        break;
                    default:
                        pattern = new byte[] { 0x20 };
                        break;
                }
            }
            IEnumerable<int> patternfound = PatternAt(inBuffer, pattern, start);
            IEnumerator<int> patternlist = patternfound.GetEnumerator();
            if (patternfound.Count() > 0)
            {
                while (patternlist.MoveNext())
                {
                    // ggf. Platz schaffen
                    int dist = lng - (pattern.GetLength(0) + deleteMore);
                    if (dist != 0)
                        inBuffer = (byte[])RemoveAt(inBuffer, patternlist.Current, dist);
                    bufferIndex = (ushort)(inBuffer.Length);
                    // token einfügen
                    if (lng > 0)
                        Array.Copy(token, 0, inBuffer, patternlist.Current, lng);
                }
            }
            return patternlist.Current;
        }

        private void generateLokConfig(byte index)
        {
            lokconfig[index].session = new byte[2];
            lokconfig[index].locid = new byte[2];
            lokconfig[index].typ = new byte[1];
            lokconfig[index].typ[0] = 0x00;
            Array.Resize(ref lokconfig[index].lokname, 0x10);
            lokconfig[index].mfxuid = new byte[8];
        }

        public void startConfigStructmfx(byte[] cnt)
        {
            lokconfig = new configstruct[1];
            generateLokConfig(0);
            // neuanmeldezähler
            lokconfig[0].session = utils.num2dec(Counter);
            // LocID
            lokconfig[0].locid = utils.num2hex(cnt[6]);
            lokconfig[0].nameIndex = 0;
            // Mfx-UID
            byte[] tmp = new byte[2];
            for (byte i = 0; i < 4; i++)
            {
                tmp = utils.num2hex(cnt[7 + i]);
                Array.Copy(tmp, 0, lokconfig[0].mfxuid, i * 2, 2);
            }
        }

        static byte[] GetBytes(string str)
        {
            byte[] bytes = Encoding.ASCII.GetBytes(str);
            return bytes;
        }

        // wird nicht bei mfx verwendet
        public void fillConfigStruct(string name, byte adr, byte type)
        {
            lokconfig = new configstruct[1];
            generateLokConfig(0);
            // neuanmeldezähler
            lokconfig[0].session = utils.num2dec(Counter);
            // LocID
            lokconfig[0].locid = utils.num2hex(adr);
            // typ +1, weil 0 ist mfx vorbehalten
            type++;
            lokconfig[0].typ[0] = type;
            // name
            lokconfig[0].lokname = GetBytes(name);
            lokconfig[0].nameIndex = (byte)name.Length;
            // Mfx-UID
            byte[] tmp = new byte[2];
            tmp = utils.num2hex(adr);
            Array.Copy(tmp, 0, lokconfig[0].mfxuid, 0, 2);
        }

        private byte fillConfigArray(byte startIndx)
        {
            readLocomotive(Cnames.path, Cnames.cfgname);
            byte cntConfig = startIndx;
            IEnumerable<int> patternfound = PatternAt(inBuffer, names.separator(), 0);
            IEnumerator<int> patternlist = patternfound.GetEnumerator();
            if (patternfound.Count() > 0)
            {
                byte entry = 5;
                int start = 0;
                while (patternlist.MoveNext())
                {
                    if (entry > 4)
                    {
                        entry = 0;
                        cntConfig++;
                        Array.Resize(ref lokconfig, cntConfig);
                        generateLokConfig((byte)(cntConfig - 1));
                    }
                    int last = patternlist.Current;
                    // pattern suchen
                    switch (entry)
                    {
                        case 0:
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].session, 0, last - start);
                            break;
                        case 1:
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].locid, 0, last - start);
                            break;
                        case 2:
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].typ, 0, last - start);
                            lokconfig[cntConfig - 1].typ[0] -= 0x30;
                            break;
                        case 3:
                            lokconfig[cntConfig - 1].nameIndex = (byte)(last - start);
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].lokname, 0, lokconfig[cntConfig - 1].nameIndex);
                            break;
                        case 4:
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].mfxuid, 0, last - start);
                            break;
                    }
                    start = last + 1;
                    entry++;
                }
            }
            return cntConfig;
        }

        private void writeConfigStruct(byte cntConfig)
        {
            // array von 0 .. cntConfig nach outbuffer und dann wegschreiben
            string tmp = string.Concat(Cnames.path, Cnames.tmpname);
            using (BinaryWriter writer = new BinaryWriter(File.Open(tmp, FileMode.Create), System.Text.Encoding.ASCII))
            {
                for (byte cntCfg = 0; cntCfg < cntConfig; cntCfg++)
                {
                    writer.Write(lokconfig[cntCfg].session, 0, lokconfig[cntCfg].session.Length);
                    writer.Write(names.separator(), 0, 1);
                    writer.Write(lokconfig[cntCfg].locid, 0, lokconfig[cntCfg].locid.Length);
                    writer.Write(names.separator(), 0, 1);
                    byte[] t = new byte[1];
                    t[0] = (byte)((byte)lokconfig[cntCfg].typ[0] + (byte)0x30);
                    writer.Write(t, 0, 1);
                    writer.Write(names.separator(), 0, 1);
                    writer.Write(lokconfig[cntCfg].lokname, 0, lokconfig[cntCfg].nameIndex);
                    writer.Write(names.separator(), 0, 1);
                    writer.Write(lokconfig[cntCfg].mfxuid, 0, lokconfig[cntCfg].mfxuid.Length);
                    writer.Write(names.separator(), 0, 1);
                }
                writer.Flush();
                writer.Close();
            }
            string cfg = string.Concat(Cnames.path, Cnames.cfgname);
            string bak = string.Concat(Cnames.path, Cnames.bakname);
            if (File.Exists(tmp) && File.Exists(cfg))
                // Replace the file
                File.Replace(tmp, cfg, bak);
            if (File.Exists(tmp))
                // Replace the file
                File.Move(tmp, cfg); // Rename the oldFileName into newFileName
        }

        void deleteDoubleEntry(byte cntConfig)
        {
            bool same;
            // nothing to compare or to delete
            if (cntConfig < 2)
                return;
            mfxBuffer = lokconfig[0].mfxuid;
            for (byte cntCfg = 1; cntCfg < cntConfig; cntCfg++)
            {
                same = true;
                for (byte m = 0; m < 8; m++)
                {
                    if (lokconfig[cntCfg].mfxuid[m] != mfxBuffer[m])
                    {
                        same = false;
                        break;
                    }
                }
                if (same == true)
                {
                    for (byte i = 0; i < cntConfig; i++)
                        if (i > cntCfg)
                            lokconfig[i - 1] = lokconfig[i];
                    cntConfig--;
                    writeConfigStruct(cntConfig);
                    break;
                }
            }
        }

        public void finishConfigStruct(bool unknown)
        {
            // 1 Lok ist die gerade gefunden
            byte cntConfig = 1;
            if (File.Exists(string.Concat(Cnames.path, Cnames.cfgname)))
            {
                cntConfig = fillConfigArray(cntConfig);
            }
            writeConfigStruct(cntConfig);
            deleteDoubleEntry(cntConfig);
            if (unknown)
                MessageBox.Show("Lok erfasst", "Lokomotiven", MessageBoxButtons.OK);
        }

        char[] typName(byte typ)
        {
            switch (typ)
            {
                case 0:
                    // mfx
                    char[] Name0 = { 'm', 'f', 'x' };
                    return Name0;
                case 1:
                    // mm2_dil8
                    char[] Name1 = { 'm', 'm', '2', '_', 'd', 'i', 'l', '8' };
                    return Name1;
                case 2:
                    // mm2_prg
                    char[] Name2 = { 'm', 'm', '2', '_', 'p', 'r', 'g' };
                    return Name2;
                case 3:
                    // dcc
                    char[] Name3 = { 'd', 'c', 'c' }; //, '\r', '\n', ' ', '.', 'x', 'p', 'r', 'o', 't', '=', '2', '\r', '\n', ' ', '.', 'p','r','o','g','m','a','s','k', '=', '0','x','3' };
                    return Name3;
                default:
                    char[] Name = { 'm', 'm', '2', '_', 'd', 'i', 'l', '8' };
                    return Name;
            };
        }

        public void generateEmptyLokList()
        {
            string f001 = string.Concat(Cnames.path, Cnames.name001);
            bool fexists001 = File.Exists(f001);
            if (!fexists001)
            {
                // Create a string array with the lines of text
                string[] lines = { "[lokomotive]", "version", " .minor=3", "session", " .id=MMMM" }; //, " " };
                // Write the string array to a new file .
                using (StreamWriter outputFile = new StreamWriter(f001))
                {
                    foreach (string line in lines)
                        outputFile.WriteLine(line);
                }
            }
            string f002 = string.Concat(Cnames.path, Cnames.name002);
            bool fexists002 = File.Exists(f002);
            if (!fexists002)
            {
                // Create a string array with the lines of text
                string[] lines = {
                    "lokomotive", " .name=NNNN", " .uid=0xOOOO", " .adresse=0xPPPP",
                    " .typ=QQQQ", " .sid=0xRRRR", " .mfxuid=0xSSSS", " .icon=TTTT", " .av=60",
                    " .bv=40", " .volume=100"," .progmask=0x3", " .vmin=13", " .vmax=100", /*" .mfxtyp=47",*/" .xprot=2", " .funktionen",
                    " ..nr=0", " ..typ=1", " .funktionen", " ..nr=1", " ..typ=51", " .funktionen", " ..nr=2", " ..typ=52",
                    " .funktionen", " ..nr=3", " ..typ=53", " .funktionen", " ..nr=4", " ..typ=54", " .funktionen",
                    " ..nr=5", " .funktionen", " ..nr=6", " .funktionen", " ..nr=7",
                    " .funktionen", " ..nr=8", " .funktionen", " ..nr=9", " .funktionen",
                    " ..nr=10", " .funktionen", " ..nr=11", " .funktionen", " ..nr=12",
                    " .funktionen", " ..nr=13", " .funktionen", " ..nr=14", " .funktionen",
                    " ..nr=15"};//, " " };
                // Write the string array to a new file 2.
                using (StreamWriter outputFile = new StreamWriter(f002))
                {
                    foreach (string line in lines)
                        outputFile.WriteLine(line);
                }
            }
            string fcfg = string.Concat(Cnames.path, Cnames.cfgname);
            bool fexistscfg = File.Exists(fcfg);
            if (!fexistscfg)
            {
                // Create a string array with the lines of text
                //session%adresse%typ%name%mfx-uid%
                string[] lines = { "00%32%1%DB CANguru%4711%", " " };
                // Write the string array to a new file .
                using (StreamWriter outputFile = new StreamWriter(fcfg))
                {
                    foreach (string line in lines)
                        outputFile.WriteLine(line);
                }
                generateLokList();
            }
        }

        public void generateLokList()
        {
            // cfg-Datei einlesen und array füllen
            byte cntConfig = 0;
            if (File.Exists(string.Concat(Cnames.path, Cnames.cfgname)))
            {
                cntConfig = fillConfigArray(cntConfig);
            }
            readLocomotive(Cnames.path, Cnames.name001);
            // MMMM - session
            insertToken(0, lokconfig[0].session, 0, 2);
            Array.Copy(inBuffer, 0, outBuffer, 0, bufferIndex);
            int last = bufferIndex;
            for (byte cntCfg = 0; cntCfg < cntConfig; cntCfg++)
            {
                readLocomotive(Cnames.path, Cnames.name002);
                int offset = 0;
                // NNNN - name
                offset = insertToken(1, lokconfig[cntCfg].lokname, offset, lokconfig[cntCfg].nameIndex);
                if (lokconfig[cntCfg].typ[0] == 0) // mfx
                {
                    // OOOO - .uid (locid  .uid=0x4006 /  .uid=0x40OOOO) insgesamt 4-stellig
                    byte[] uid = new byte[4];
                    uid[0] = 0x34;
                    uid[1] = 0x30;
                    uid[2] = lokconfig[cntCfg].locid[0];
                    uid[3] = lokconfig[cntCfg].locid[1];
                    offset = insertToken(2, uid, offset, 4);
                    // PPPP - .adresse ( .adresse = 0x6 / .adresse = 0xPPPP) n-stellig
                    offset = insertToken(3, lokconfig[cntCfg].locid, offset, 2);
                    // QQQQ - .typName=mfx / .typName=mm2_dil8
                    byte[] name = Encoding.GetEncoding("UTF-8").GetBytes(typName(lokconfig[cntCfg].typ[0]));
                    offset = insertToken(4, name, offset, (byte)name.Length);
                    // RRRR - .sid=0x6 / .sid=0xQQQQ wie adresse
                    offset = insertToken(5, lokconfig[cntCfg].locid, offset, 2);
                    // SSSS - .mfxuid=0x7cfdc941 / .mfxuid=0xRRRR
                    offset = insertToken(6, lokconfig[cntCfg].mfxuid, offset, 8);
                    // TTTT - .icon
                    offset = insertToken(7, lokconfig[cntCfg].lokname, offset, lokconfig[cntCfg].nameIndex);
                }
                else // andere als mfx
                {
                    // OOOO - .uid (locid  .uid=0x4006 /  .uid=0x40OOOO) insgesamt 4-stellig
                    if (lokconfig[cntCfg].typ[0] == 0x03) //dcc
                    {
                        byte[] dccLocid = { 0x43, 0x30, 0x30, 0x30, };
                        Array.Copy(lokconfig[cntCfg].locid, 0, dccLocid, 0x02, 0x02);
                        offset = insertToken(2, dccLocid, offset, 4);
                    }
                    else
                        offset = insertToken(2, lokconfig[cntCfg].locid, offset, 2);
                    // PPPP - .adresse ( .adresse = 0x6 / .adresse = 0xPPPP) n-stellig
                    offset = insertToken(3, lokconfig[cntCfg].locid, offset, 2);
                    // QQQQ - .typName=mfx / .typName=mm2_dil8
                    byte[] name = Encoding.GetEncoding("UTF-8").GetBytes(typName(lokconfig[cntCfg].typ[0]));
                    offset = insertToken(4, name, offset, (byte)name.Length);
                    // TTTT - .icon
                    offset = insertToken(7, lokconfig[cntCfg].lokname, offset, lokconfig[cntCfg].nameIndex);
                    // RRRR - .sid=0x6 / .sid=0xQQQQ wie adresse
                    //     byte[] sid = { 0x37, 0x33 };
                    //     offset = insertToken(5, sid, offset, 2);
                    byte[] dummy = { 0x30 };
                    offset = insertToken(10, dummy, offset, 0);
                    // SSSS -  " .mfxuid=0xSSSS" delete
                    // string mfxToken = " .mfxuid=0xSSSS";
                    //offset = insertToken(6, mfxuid, offset, 1);
                    offset = insertToken(11, dummy, offset, 0);
                }
                Array.Copy(inBuffer, 0, outBuffer, last, bufferIndex);
                last += bufferIndex;
            }
            string tmp = string.Concat(Cnames.path, Cnames.tmpname);
            //            using (BinaryWriter writer = new BinaryWriter(File.Open(tmp, FileMode.Create)))
            using (BinaryWriter writer = new BinaryWriter(File.Open(tmp, FileMode.Create), System.Text.Encoding.ASCII))
            {
                writer.Write(outBuffer, 0, last);
                writer.Flush();
                writer.Close();
            }
            string cs2 = string.Concat(Cnames.path, Cnames.cs2name);
            string bak = string.Concat(Cnames.path, Cnames.bakname);
            if (File.Exists(tmp) && File.Exists(cs2))
                // Replace the file.
                File.Replace(tmp, cs2, bak);
            if (File.Exists(tmp))
                // Replace the file
                File.Move(tmp, cs2); // Rename the oldFileName into newFileName
        }
        public void generateLokListwithListbox(ListBox lb)
        {
            generateLokList();
            editConfigStruct(lb);
            MessageBox.Show("Lokliste angelegt", "Lokomotiven", MessageBoxButtons.OK);
        }

        public static byte getLocid(int line, byte no)
        {
            return lokconfig[line].locid[no];
        }

        public static byte getMFXUID(int line, byte no)
        {
            return lokconfig[line].mfxuid[no];
        }

        public void editConfigStruct(ListBox lb)
        {
            lb.Items.Clear();
            // cfg-Datei einlesen und array füllen
            byte cntConfig = 0;
            if (File.Exists(string.Concat(Cnames.path, Cnames.cfgname)))
            {
                cntConfig = fillConfigArray(cntConfig);
            }
            for (byte itemNo = 0; itemNo < cntConfig; itemNo++)
            {
                string name = "Name: " + Encoding.UTF8.GetString(lokconfig[itemNo].lokname, 0, lokconfig[itemNo].nameIndex);
                string locid = "LocID: " + Encoding.UTF8.GetString(lokconfig[itemNo].locid, 0, 2);
                string mfxuid = "mfxUID: " + "0x" + Encoding.UTF8.GetString(lokconfig[itemNo].mfxuid, 0, 8);
                string item = name + " - " + locid + " - " + mfxuid;
                lb.Items.Add(item);
            }
            if (cntConfig>0)
                lb.SetSelected(0, true);
        }

        public void getLokName(byte ch)
        {
            lokconfig[0].lokname[lokconfig[0].nameIndex] = ch;
            if (ch != 0x00)
                lokconfig[0].nameIndex++;
        }

        public void delConfigStruct(ListBox lb)
        {
            if (lb.Items.Count == 0)
            {
                MessageBox.Show("Bitte zunächst Lokliste füllen", "Fehler", MessageBoxButtons.OK);
            }
            else
            {
                // Get the currently selected item in the ListBox.
                int selIndex = lb.SelectedIndex;
                //
                if (selIndex == -1)
                    MessageBox.Show("Bitte eine Lok auswählen", "Fehler", MessageBoxButtons.OK);
                else
                {
                    byte cntConfig = (byte)lb.Items.Count;
                    for (byte i = 0; i < cntConfig; i++)
                        if (i > selIndex)
                            lokconfig[i - 1] = lokconfig[i];
                    cntConfig--;
                    writeConfigStruct(cntConfig);
                    // Listbox neu laden
                    editConfigStruct(lb);
                }
            }
        }

        public void sendBufferUncompressed(byte[] cnt, UdpClient CANClient)
        {
            // 5 6 7 8 9 A B C
            // 1 2 3 4 5 Y X X
            byte lineNo = cnt[0x0A];
            int buffLen = BitConverter.ToUInt16(cnt, 0x0B);
            byte[] tmpBuffer = new byte[buffLen];
            Array.Clear(tmpBuffer, 0, buffLen);
            int toCopy = 0;
            int inBufferIndex = lineNo * buffLen;
            if ((bufferIndex - inBufferIndex) < buffLen)
            {
                toCopy = bufferIndex - inBufferIndex;
            }
            else
            {
                toCopy = buffLen;
            }
            Array.Copy(inBuffer, inBufferIndex, tmpBuffer, 0, toCopy);
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(tmpBuffer, toCopy);
        }
    }

}
