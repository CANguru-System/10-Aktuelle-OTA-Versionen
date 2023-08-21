using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using zlib;

namespace CANguruX
{
    class compress
    {
        const int CHUNK = 0xFFFF;        // maximum MTU
        public static byte[] outBuffer = new byte[CHUNK];
        static Int32 bufferIndex = 0;
        static Int32 deflated_size;
        static public bool bcompress;

        public static void CompressData(byte[] inData, out byte[] outData)
        {
            ZStream stream = new ZStream();
            byte[] outZData = new byte[CHUNK];
            stream.next_in = inData;
            stream.avail_in = bufferIndex;
            stream.next_out = outZData;
            stream.avail_out = CHUNK;

            // deflateInit2(strm, level, Z_DEFLATED, bits, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
            // CALL_ZLIB(deflateInit2(strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY));
            // CALL_ZLIB(deflate(&strm, Z_FINISH));
            const int BITS = 15; // full 32k=15 outbound buffer 
            stream.deflateInit(zlibConst.Z_DEFAULT_COMPRESSION, BITS);

            stream.deflate(zlibConst.Z_FINISH);
            stream.deflateEnd();

            byte[] output = new byte[stream.total_out + 4];
            Buffer.BlockCopy(stream.next_out, 0, output, 4, (int)stream.total_out);
            byte[] tmpbyte4 = new byte[4];
            tmpbyte4 = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(bufferIndex));
            Array.Copy(tmpbyte4, output, 4);
            outData = output;
            deflated_size = CHUNK - stream.avail_out;
        }

        public static void compressTheData(string m_filename)
        {
            byte[] inBuffer = new byte[CHUNK];
            bufferIndex = 0;
            // Using BinaryReader
            FileStream fs = new FileStream(m_filename, FileMode.Open);
            using (BinaryReader reader = new BinaryReader(fs))
            {
                // Read bytes
                reader.BaseStream.Position = 0;
                while (reader.BaseStream.Position < reader.BaseStream.Length)
                {
                    inBuffer[bufferIndex] = reader.ReadByte();
                    bufferIndex++;
                    if (bufferIndex >= CHUNK)
                    {
                        deflated_size = 0;
                        return;
                    }
                }
            }
            CompressData(inBuffer, out outBuffer);
        }

        public static void sendBufferCompressed(byte[] cnt, UdpClient CANClient)
        {
            Int32 compressedBufferLength = outBuffer.Length;
            // 5 6 7 8 9 A B C
            // 1 2 3 4 5 Y X X
            byte buffNo2Send = cnt[0x0A];
            int buffLen2Send = BitConverter.ToUInt16(cnt, 0x0B);
            // die Bridge hat einen Buffer mit der Länge buffLen
            // so groß wird auch der temporäre Buffer
            byte[] tmpBuffer = new byte[buffLen2Send];
            // der temproräre Buffer wird mit Nullen überschrieben
            Array.Clear(tmpBuffer, 0, buffLen2Send);
            // wieviele Bytes sind in den temorären Buffer zu kopieren und dann zu übertragen?
            // inBufferIndex soviele wurden bereits übertragen bzw. ab hier geht es weiter mit der Übertragung
            Int32 inBufferIndex = buffNo2Send * buffLen2Send;
            // rest2Send ist die Differenz zwischen compressedBufferLength (soviele bytes gibt es insgesamt)
            // und dem Anteil, der bereits übertragen wurde
            Int32 rest2Send = compressedBufferLength - inBufferIndex;
            // es dürfen höchstens buffLen2Send bytes übertragen werden
            if (rest2Send < 0)
                rest2Send *= -1;
            Int32 bytes2Copy = Math.Min(rest2Send, buffLen2Send);
            Array.Copy(outBuffer, inBufferIndex, tmpBuffer, 0, bytes2Copy);
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(tmpBuffer, bytes2Copy);
        }

    public int getDeflatedSize()
        {
            return deflated_size;
        }
    }
}
