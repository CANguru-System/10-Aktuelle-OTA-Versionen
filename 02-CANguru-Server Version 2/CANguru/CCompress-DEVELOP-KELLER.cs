using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using ZLIB;

namespace CANguruX
{
    class compress
    {
        static public bool bcompress;
        const int CHUNK = 0x8000;        // maximum MTU
        public static byte[] outBuffer = new byte[CHUNK];
        static Int32 bufferIndex = 0;
        static int deflated_size;
        public static void CopyStream(System.IO.Stream input, System.IO.Stream output)
        {
            byte[] buffer = new byte[CHUNK];
            int len;
            while ((len = input.Read(buffer, 0, CHUNK)) > 0)
            {
                output.Write(buffer, 0, len);
            }
            output.Flush();
        }

        public static void CompressData(byte[] inData, out byte[] outData)
        {
            ZLIB.ZLIBStream stream = new ZLIBStream();
            byte[] outZData = new byte[CHUNK];
            stream.next_in = inData;
            stream.avail_in = bufferIndex;
            stream.next_out = outZData;
            stream.avail_out = CHUNK;

            // deflateInit2(strm, level, Z_DEFLATED, bits, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
            // CALL_ZLIB(deflateInit2(strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY));
            // CALL_ZLIB(deflate(&strm, Z_FINISH));
            const int BITS = 15; // full 32k=15 outbound buffer 
            stream.deflateInit(ZLIB.zlibConst.Z_DEFAULT_COMPRESSION, BITS);

            stream.deflate(ZLIB.zlibConst.Z_FINISH);
            stream.deflateEnd();

            byte[] output = new byte[stream.total_out+4];
            Buffer.BlockCopy(stream.next_out, 0, output, 4, (int)stream.total_out);
            bufferIndex = IPAddress.HostToNetworkOrder(bufferIndex);
            byte[] tmpbyte4 = new byte[4];
            tmpbyte4 = BitConverter.GetBytes(bufferIndex);
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
                }
            }
            CompressData(inBuffer, out outBuffer);
        }

        public int getDeflatedSize()
        {
            return deflated_size; // outBuffer.Length + 4;
        }

        public static void sendBufferCompressed(byte[] cnt, UdpClient CANClient)
        {
            byte lineNo = cnt[9];
            int buffLen = BitConverter.ToUInt16(cnt, 10);
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
            Array.Copy(outBuffer, inBufferIndex, tmpBuffer, 0, toCopy);
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(tmpBuffer, toCopy);
        }
    }
}

