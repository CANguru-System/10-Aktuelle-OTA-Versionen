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

        private byte oneByte(byte n)
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

        public static ushort hex2num(byte[] ascii, byte num)
        {
            byte i;
            ushort val = 0;
            for (i = 0; i < num; i++)
            {
                byte c = ascii[i];
                // Hex-Ziffer auf ihren Wert abbilden
                if (c >= '0' && c <= '9')
                    c -= (byte)'0';
                else if (c >= 'A' && c <= 'F') c -= 'A' - 10;
                else if (c >= 'a' && c <= 'f') c -= 'a' - 10;
                val = (ushort)(16 * val + c);
            }
            return val;
        }
    }
}
