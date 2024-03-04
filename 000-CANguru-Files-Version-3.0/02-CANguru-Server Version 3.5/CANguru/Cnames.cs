using System;

namespace CANguruX
{
    class Cnames
    {
        public const byte cntCS2Files = 10;
        public const byte shortnameLng = 7;
        public const byte shortnameLng2transfer = shortnameLng - 2;
        // init array
        // Dateien mit "0" sind verzichtbar
        static public string[,] cs2Files = new string[cntCS2Files, 3] {
            { "loks   ", @"\lokomotive.cs2", "0"},
            { "mags   ", @"\magnetartikel.cs2", "0"},
            { "fs     ", @"\fahrstrassen.cs2", "0"},
            { "gbs    ", @"\gleisbild.cs2", "0"},
            { "prefs  ", @"\prefs.cs2", "0"},
            { "ger    ", @"\geraet.vrs", "1"},
            { "lokstat", @"\lokomotive.sr2", "0"},
            { "magstat", @"\magnetartikel.sr2", "0"},
            { "gbsstat", @"\gleisbild.sr2", "1"},
            { "fsstat ", @"\fahrstrassen.sr2", "0"},
        };
        static public string path = @"c:\CANguru";
        static public string ininame = @"\CANguru.ini";
        static public string saveTxtname = @"\CANguru.txt";
        static public string cs2name = @"\lokomotive.cs2";
        static public string name001 = @"\lokomotive.001";
        static public string name002 = @"\lokomotive.002";
        static public string tmpname = @"\lokomotive.tmp";
        static public string bakname = @"\lokomotive.bak";
        static public string cfgname = @"\lokomotive.cfg";
        static public string glsbild = @"\gleisbild.cs2";
        public const byte lngFrame = 13;
        public const byte maxCANgurus = 20;
        // der Lichtdecodder hat momentan 10 Zeilen; aber lieber noch einige drauf
        public const byte maxConfigLines = 20;
        // IN is even
        static public Int32 localPortDelta = 2;      // local port to listen on
        static public Int32 portinCAN = 15730 + localPortDelta;
        static public Int32 portoutCAN = 15731 + localPortDelta;
        static public string IP_CAN = "192.168.178.71";
        public const int port = 23;
        public const byte toCAN = 1;
        public byte[] sep = { 0x25 }; // %
        public byte[] separator()
        {
            return sep;
        }
    }
}
