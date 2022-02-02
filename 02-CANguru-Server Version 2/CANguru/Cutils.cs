using System;
using System.Net;

namespace CANguruX
{
    class Cutils
    {
        private const byte number = 0x30;
        private const byte letter = 0x37;

        // Public constructor
        public Cutils()
        {
        }

        public static byte oneByte(byte n)
        {
            if (n < 10)
                return (byte)(n + number);
            else
                return (byte)(n + letter);
        }

        public byte[] num2hex(byte num)
        {
            byte[] hexarray = { 0x00, 0x00 };
            hexarray[0] = oneByte((byte)((num & 0xF0) >> 4));
            hexarray[1] = oneByte((byte)(num & 0x0F));
            return hexarray;
        }

        public byte[] num2dec(byte num)
        {
            byte[] hexarray = { 0x00, 0x00 };
            hexarray[0] = oneByte((byte)(num / 10));
            hexarray[1] = oneByte((byte)(num % 10));
            return hexarray;
        }

        public static byte hex2num(byte[] ascii)
        {
            byte i;
            byte num = 2;
            byte val = 0;
            for (i = 0; i < num; i++)
            {
                byte c = ascii[i];
                // Hex-Ziffer auf ihren Wert abbilden
                if (c >= '0' && c <= '9')
                    c -= (byte)'0';
                else if (c >= 'A' && c <= 'F') c -= 'A' - 10;
                else if (c >= 'a' && c <= 'f') c -= 'a' - 10;
                val = (byte) (16 * val + c);
            }
            return val;
        }
        public static string DecimalToHexadecimal(int dec)
        {
            const int baseCount = 16;
            if (dec < 1) return "0";

            int hex = dec;
            string hexStr = string.Empty;

            while (dec > 0)
            {
                hex = dec % baseCount;
                if (hex < 10)
                    hexStr = hexStr.Insert(0, Convert.ToChar(hex + '0').ToString());
                else
                    hexStr = hexStr.Insert(0, Convert.ToChar(hex + '7').ToString());

                dec /= baseCount;
            }

            return hexStr;
        }

        public static byte[] StringToByteArray(string hex)
        {
            byte[] arr = new byte[hex.Length >> 1];

            for (int i = 0; i < hex.Length >> 1; ++i)
            {
                arr[i] = (byte)((GetHexVal(hex[i << 1]) << 4) + (GetHexVal(hex[(i << 1) + 1])));
            }

            return arr;
        }

        public static int GetHexVal(char hex)
        {
            int val = (int)hex;
            //For uppercase A-F letters:
            //return val - (val < 58 ? 48 : 55);
            //For lowercase a-f letters:
            //return val - (val < 58 ? 48 : 87);
            //Or the two combined, but a bit slower:
            return val - (val < 58 ? 48 : (val < 97 ? 55 : 87));
        }
    }
}
