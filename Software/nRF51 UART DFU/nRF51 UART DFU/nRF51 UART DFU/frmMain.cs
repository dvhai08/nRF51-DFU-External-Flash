using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.IO.Ports;
using System.Resources;

namespace nRF51_UART_DFU
{
    public partial class frmMain : Form
    {
        const int DEFAULT_COMM_TIMEOUT = 500;
        const int MAX_COMM_RETRY = 1;

        const byte COMM_START_CODE = 0xCA;

        const byte READ_FIRMWARE_VERSION_OPCODE = 0x11;
        const byte DOWNLOAD_FIRMWARE_OPCODE = 0x12;
        const byte DISABLE_LOG_OPCODE = 0x01;
        const byte ENABLE_LOG_OPCODE = 0x02;
        const byte READ_LOG_OPCODE = 0x03;
        const byte ENABLE_GPS_LOG_OPCODE = 0x04;
        const byte ENABLE_GSM_LOG_OPCODE = 0x05;
        const byte ENABLE_MCU_LOG_OPCODE = 0x06;
        const byte READ_CONFIG_OPCODE = 0x0C;
        const byte WRITE_CONFIG_OPCODE = 0x0D;
        const byte SET_CAMERA_ID_OPCODE = 0x13;

        const UInt32 DEFAULT_PACKET_NO = 0xA5A5A5A5;
        const UInt32 ERROR_PACKET_NO = 0xFFFFFFFF;
        const UInt32 FINISH_PACKET_NO = 0x5A5A5A5A;
        const int START_CODE_OFFSET = 0;
        const int LENGTH_OFFSET = 1;
        const int OPCODE_OFFSET = 3;
        const int DATA_OFFSET = 4;
        const int PACKET_NO_OFFSET = 4;
        const int DATA_WITH_PACKET_NO_OFFSET = 8;
        const int SIMPLE_CRC_WITH_PACKET_NO_OFFSET = 8;
        const int LENGTH_SIZE = 2;
        const int PACKET_NO_SIZE = 4;
        const int HEADER_SIZE = OPCODE_OFFSET + 1;
        const int CRC_SIZE = 1;
        const int SIMPLE_PACKET_SIZE = HEADER_SIZE + PACKET_NO_SIZE + CRC_SIZE;
        const int MAX_DATA_SIZE = 507;
        const int SMS_SIZE = 32;
        const byte UPLOAD_LOG_OPCODE = 0x0F;

        SerialPort serialPort;
        int currentOperation;
        bool downloadingFirmware;
        bool readingReportFile;

        // variables defined for localized string
        // control strings
        ResourceString APP_CAPTION_STR = new ResourceString();
        ResourceString DATA_SERVER_CMBBOX_ITEM_STR = new ResourceString();
        ResourceString FIRMWARE_SERVER_CMBBOX_ITEM_STR = new ResourceString();
        ResourceString IMAGE_SERVER_CMBBOX_ITEM_STR = new ResourceString();
        ResourceString INTERVAL_TRIGGER_SOURCE_CHKLIST_ITEM_STR = new ResourceString();
        ResourceString CONNECTED_STR = new ResourceString();
        ResourceString DISCONNECTED_STR = new ResourceString();
        ResourceString RETRIEVING_FILE_STR = new ResourceString();
        ResourceString OPEN_FILE_DIALOG_TITLE_STR = new ResourceString();
        ResourceString SAVE_FILE_DIALOG_TITLE_STR = new ResourceString();
        ResourceString OPEN_FOLDER_DIALOG_TITLE_STR = new ResourceString();
        ResourceString DOWNLOADING_FIRMWARE_STR = new ResourceString();
        ResourceString LOG_INTERVAL_1_MIN_STR = new ResourceString();
        ResourceString LOG_INTERVAL_5_MIN_STR = new ResourceString();
        ResourceString LOG_INTERVAL_10_MIN_STR = new ResourceString();
        ResourceString LOG_INTERVAL_30_MIN_STR = new ResourceString();
        // message strings
        ResourceString READ_CONFIG_SUCCESS_STR = new ResourceString();
        ResourceString READ_CONFIG_FAILED_STR = new ResourceString();
        ResourceString WRITE_CONFIG_SUCCESS_STR = new ResourceString();
        ResourceString WRITE_CONFIG_FALIED_STR = new ResourceString();
        ResourceString FORMAT_SDCARD_WARNING_STR = new ResourceString();
        ResourceString KML_SAVE_SUCCESS_STR = new ResourceString();
        ResourceString CONFIG_SAVE_SUCCESS_STR = new ResourceString();
        ResourceString CONFIG_LOADED_STR = new ResourceString();
        ResourceString CANNOT_OPEN_COMPORT_STR = new ResourceString();
        ResourceString FIRMWARE_DOWNLOADED_SUCCESS_STR = new ResourceString();
        ResourceString FIRMWARE_DOWNLOADED_FAILED_STR = new ResourceString();
        ResourceString CANNOT_OPEN_FILE_STR = new ResourceString();
        ResourceString FILE_NAME_IS_INVALID_WARNING_STR = new ResourceString();
        ResourceString SET_CAMERA_ID_SUCCESS_STR = new ResourceString();
        ResourceString SET_CAMERA_ID_FAILED_STR = new ResourceString();
        List<ResourceString> localizedStrings;

        byte[] comOutBuffer = new byte[1024];
        byte[] comInBuffer = new byte[1024];
        byte[] comTempBuffer = new byte[1024];
        byte[] comRetryBuffer;
        BinaryReader firmwareReader;
        int comRetryTimeout;
        int comRetryCount;
        int comBufIndex = 0;

        UInt32 fDeviceBuffSize;

        System.Timers.Timer comTimeoutTimer;
        public frmMain()
        {
            InitializeComponent();
        }

        private void btnOpen_Click(object sender, EventArgs e)
        {
            string fileName;
            DialogResult res;

            openFileDialog1.Filter = "BIN file (*.BIN)|*.BIN";
            res = openFileDialog1.ShowDialog();
            if (res != DialogResult.OK)
            {
                return;
            }
            fileName = openFileDialog1.FileName;
            if (!File.Exists(fileName))
            {
                return;
            }
            firmwarePathTxtBox.Text = fileName;
        }

        private void btnDownload_Click(object sender, EventArgs e)
        {
            string fileName;
            byte[] fileSize;
            byte[] fileCRC;

            fileName = firmwarePathTxtBox.Text;
            if (!File.Exists(fileName))
            {
                MessageBox.Show(FILE_NAME_IS_INVALID_WARNING_STR.Text, APP_CAPTION_STR.Text);
                return;
            }
            if (serialPort == null ||  !serialPort.IsOpen)
            {
                return;
            }

            if (firmwareReader != null)
            {
                try
                {
                    firmwareReader.Close();
                }
                catch (Exception)
                { }
            }

            fileCRC = BitConverter.GetBytes(CalcFileCRC(firmwarePathTxtBox.Text));
            try
            {
                firmwareReader = new BinaryReader(new FileStream(firmwarePathTxtBox.Text, FileMode.Open));
            }
            catch (Exception)
            {
                MessageBox.Show(CANNOT_OPEN_FILE_STR.Text + fileName, APP_CAPTION_STR.Text);
                return;
            }

            downloadingFirmware = true;
            EnableDeviceCmd(false);
            fileSize = BitConverter.GetBytes(firmwareReader.BaseStream.Length);
            CreateCOMBuffer(PACKET_NO_SIZE + 8, DOWNLOAD_FIRMWARE_OPCODE, DEFAULT_PACKET_NO);
            Array.Copy(fileSize, 0, comOutBuffer, DATA_WITH_PACKET_NO_OFFSET, 4);
            Array.Copy(fileCRC, 0, comOutBuffer, DATA_WITH_PACKET_NO_OFFSET + 4, 4);
            comOutBuffer[DATA_WITH_PACKET_NO_OFFSET + 8] = calcCRC(comOutBuffer, DATA_OFFSET, PACKET_NO_SIZE + 8);
            SendCOMData(comOutBuffer, 0, HEADER_SIZE + PACKET_NO_SIZE + 8 + CRC_SIZE);
            retrieveLogPrgBar.Maximum = (int)(firmwareReader.BaseStream.Length / 100);
            retrieveLogStatusLbl.Text = DOWNLOADING_FIRMWARE_STR.Text;
            UpdateProgressBar(true);
        }

        private void CreateCOMBuffer(int length, byte opcode, UInt32 packetNo)
        {
            Array.Clear(comOutBuffer, 0, comOutBuffer.Length);
            comOutBuffer[START_CODE_OFFSET] = COMM_START_CODE;
            Array.Copy(BitConverter.GetBytes(length), 0, comOutBuffer, LENGTH_OFFSET, LENGTH_SIZE);
            comOutBuffer[OPCODE_OFFSET] = opcode;
            Array.Copy(BitConverter.GetBytes(packetNo), 0, comOutBuffer, PACKET_NO_OFFSET, PACKET_NO_SIZE);
            currentOperation = opcode;
        }

        private void SendCOMData(byte[] buffer, int offset, int count, int timeout = DEFAULT_COMM_TIMEOUT)
        {
            int retryCount;

            serialPort.WriteTimeout = timeout;
            retryCount = 0;
            while (true)
            {
                try
                {
                    serialPort.Write(buffer, offset, count);
                    break;
                }
                catch (Exception)
                {
                    if (retryCount++ > MAX_COMM_RETRY)
                    {
                        ReportCOMWriteTimedOut();
                        return;
                    }
                }
            }
            if (timeout > DEFAULT_COMM_TIMEOUT)
            {
                comRetryBuffer = new byte[count];
                Array.Copy(buffer, offset, comRetryBuffer, 0, count);
                comRetryTimeout = timeout;
                comTimeoutTimer.Stop();
                comTimeoutTimer.Interval = timeout;
                comTimeoutTimer.Start();
            }
        }

        void UpdateProgressBar(bool visible)
        {
            if (visible == false)
            {
                retrieveLogPrgBar.Value = 0;
                retrieveLogStatusLbl.Text = "0%";
            }
            retrieveLogPrgBar.Visible = visible;
            retrieveLogStatusLbl.Visible = visible;
            retrieveLogPercentLbl.Visible = visible;
        }

        private byte calcCRC(byte[] buf, int index, int length)
        {
            byte crc;

            crc = 0;
            for (int i = index; i < index + length; i++)
            {
                crc += buf[i];
            }

            return crc;
        }


        private void EnableDeviceCmd(bool enabled)
        {
            //deviceCmdGBox.Enabled = enabled;
            //EnableCommTypeChange(enabled);
            //reportGrpBox.Enabled = enabled;
            //readFirmwareVersionBtn.Enabled = enabled;
            //downloadFirmwareBtn.Enabled = enabled;
        }

        private uint calcCRC32(byte[] buf, int index, int length)
        {
            uint crc;

            crc = 0;
            for (int i = index; i < index + length; i++)
            {
                crc += buf[i];
            }

            return crc;
        }
        private uint CalcFileCRC(string fileName)
        {
            List<byte> temp;
            uint crc;

            temp = ReadAllByteInFile(fileName);
            crc = calcCRC32(temp.ToArray(), 0, temp.Count);
            temp.Clear();
            temp = null;

            return crc;
        }
        private List<byte> ReadAllByteInFile(string fileName)
        {
            BinaryReader reader;
            long fileSize;
            List<byte> buffer;
            byte[] tempBuf;
            int bufIndex;

            reader = null;
            try
            {
                reader = new BinaryReader(new FileStream(fileName, FileMode.Open));
            }
            catch (Exception)
            {
                return null;
            }
            if (reader == null)
            {
                return null;
            }

            fileSize = reader.BaseStream.Length;
            buffer = new List<byte>();
            bufIndex = 0;
            while (bufIndex < fileSize)
            {
                tempBuf = reader.ReadBytes(1024);
                buffer.AddRange(tempBuf);
                bufIndex += tempBuf.Length;
            }
            reader.Close();
            reader = null;

            return buffer;
        }

        private void frmMain_Load(object sender, EventArgs e)
        {
            comTimeoutTimer = new System.Timers.Timer();
            comTimeoutTimer.AutoReset = false;
            comTimeoutTimer.Elapsed += new System.Timers.ElapsedEventHandler(comTimeoutTimer_Elapsed);

            foreach (string portName in SerialPort.GetPortNames())
            {
                comPortCmbBox.Items.Add(portName);
            }
            if (comPortCmbBox.Items.Count > 0)
            {
                comPortCmbBox.SelectedIndex = 0;
            }

            // initialize localized strings
            InitLocalizedStrings();
        }

        private void comTimeoutTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            if (comRetryCount++ > MAX_COMM_RETRY)
            {
                comRetryCount = 0;
                this.Invoke((MethodInvoker)delegate
                {
                    //ReportCOMWriteTimedOut();
                });
                return;
            }
            SendCOMData(comRetryBuffer, 0, comRetryBuffer.Length, comRetryTimeout);
        }


        private void ReportCOMWriteTimedOut()
        {
            if (currentOperation == UPLOAD_LOG_OPCODE)
            {
                readingReportFile = false;
                return;
            }
            if (currentOperation == READ_CONFIG_OPCODE)
            {
                MessageBox.Show(READ_CONFIG_FAILED_STR.Text, APP_CAPTION_STR.Text);
                EnableDeviceCmd(true);
                return;
            }
            if (currentOperation == WRITE_CONFIG_OPCODE)
            {
                MessageBox.Show(WRITE_CONFIG_FALIED_STR.Text, APP_CAPTION_STR.Text);
                EnableDeviceCmd(true);
                return;
            }
            if (currentOperation == DOWNLOAD_FIRMWARE_OPCODE)
            {
                if (downloadingFirmware)
                {
                    downloadingFirmware = false;
                    EnableDeviceCmd(true);
                    UpdateProgressBar(false);
                    MessageBox.Show(FIRMWARE_DOWNLOADED_FAILED_STR.Text, APP_CAPTION_STR.Text);
                }
                return;
            }
        }

        private void btnOpenPort_Click(object sender, EventArgs e)
        {
            if ((serialPort != null) && (serialPort.IsOpen))
            {
                serialPort.Close();
            }

            // open COM Port
            serialPort = new SerialPort(comPortCmbBox.Text);
            serialPort.BaudRate = 115200;
            //serialPort.WriteTimeout = 1000;
            try
            {
                serialPort.Open();
            }
            catch (Exception)
            {
                // "Cannot open port "
                MessageBox.Show(CANNOT_OPEN_COMPORT_STR.Text + serialPort.PortName, APP_CAPTION_STR.Text);
                return;
            }
            //commuTypeLbl.Text = serialPort.PortName;
            //communicationType = COM_COMMUNICATION;

            // stop USB communication
            //CloseHIDDevice();
            //HID.UnregisterDeviceNotification(notificationHandle);

            // start COM communication
            comBufIndex = 0;
            serialPort.DataReceived += new SerialDataReceivedEventHandler(serialPort_DataReceived);
            //ReadConfig();
        }

        private void serialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            byte[] b;
            int length;
            int copyIndex;

            length = serialPort.BytesToRead;
            b = new byte[length];
            serialPort.Read(b, 0, length);
            if (comBufIndex == 0)
            {
                copyIndex = -1;
                for (int i = 0; i < b.Length; i++)
                {
                    if (b[i] == COMM_START_CODE)
                    {
                        copyIndex = i;
                        break;
                    }
                }
                if (copyIndex < 0)
                {
                    return;
                }
                Array.Clear(comTempBuffer, 0, comTempBuffer.Length);
                Array.Copy(b, copyIndex, comTempBuffer, 0, b.Length - copyIndex);
                comBufIndex += b.Length - copyIndex;
            }
            else
            {
                Array.Copy(b, 0, comTempBuffer, comBufIndex, b.Length);
                comBufIndex += b.Length;
            }
            if (comBufIndex >= (BitConverter.ToInt16(comTempBuffer, LENGTH_OFFSET) + HEADER_SIZE + CRC_SIZE))
            {
                comBufIndex = 0;
                Array.Copy(comTempBuffer, comInBuffer, comInBuffer.Length);
                ProcessReceivedCOMData();
            }
        }
        private void ProcessReceivedCOMData()
        {
            byte crc;
            int length;
            byte opcode;
            UInt32 packetNo;
            byte[] bTemp;

            // should have something in the buffer
            length = BitConverter.ToInt16(comInBuffer, LENGTH_OFFSET);
            if (length <= 0)
            {
                return;
            }
            // validate the CRC
            crc = comInBuffer[HEADER_SIZE + length];
            if (crc != calcCRC(comInBuffer, DATA_OFFSET, length))
            {
                return;
            }

            opcode = comInBuffer[OPCODE_OFFSET];
            switch (opcode)
            {
                case READ_LOG_OPCODE:             // debug data
                    //this.Invoke(new MethodInvoker(UpdateTextBox));
                    break;

                case WRITE_CONFIG_OPCODE:
                    //comTimeoutTimer.Stop();
                    //packetNo = BitConverter.ToUInt32(comInBuffer, PACKET_NO_OFFSET);

                    //if (packetNo == FINISH_PACKET_NO)
                    //{
                    //    MessageBox.Show(WRITE_CONFIG_SUCCESS_STR.Text, APP_CAPTION_STR.Text);
                    //    this.Invoke((MethodInvoker)delegate
                    //    {
                    //        EnableDeviceCmd(true);
                    //    });
                    //    return;
                    //}
                    //if (packetNo == ERROR_PACKET_NO)
                    //{
                    //    return;
                    //}
                    //if (packetNo == DEFAULT_PACKET_NO)
                    //{
                    //    cfgDataSize = BitConverter.ToUInt32(comInBuffer, DATA_WITH_PACKET_NO_OFFSET);
                    //    cfgDeviceBuffSize = BitConverter.ToUInt32(comInBuffer, DATA_WITH_PACKET_NO_OFFSET + 4);
                    //    cfgDataOffset = 0;
                    //    length = (int)cfgDeviceBuffSize;
                    //}
                    //else
                    //{
                    //    cfgNumByteToSend = BitConverter.ToUInt32(comInBuffer, DATA_WITH_PACKET_NO_OFFSET);
                    //    cfgDataOffset = (int)packetNo;
                    //    length = (int)cfgNumByteToSend;
                    //}

                    //if (cfgDataOffset >= cfgDataSize)         // last packet
                    //{
                    //    MessageBox.Show(WRITE_CONFIG_SUCCESS_STR.Text, APP_CAPTION_STR.Text);
                    //    this.Invoke((MethodInvoker)delegate
                    //    {
                    //        EnableDeviceCmd(true);
                    //    });
                    //    return;
                    //}
                    //CreateCOMBuffer(PACKET_NO_SIZE + length, WRITE_CONFIG_OPCODE, (UInt32)cfgDataOffset);
                    //Array.Copy(cfgData, cfgDataOffset, comOutBuffer, DATA_WITH_PACKET_NO_OFFSET, length);
                    //comOutBuffer[DATA_WITH_PACKET_NO_OFFSET + length] = calcCRC(comOutBuffer, DATA_OFFSET, PACKET_NO_SIZE + length);
                    //SendCOMData(comOutBuffer, 0, HEADER_SIZE + PACKET_NO_SIZE + length + CRC_SIZE);
                    break;

                case READ_CONFIG_OPCODE:          // config data
                    //comTimeoutTimer.Stop();
                    //packetNo = BitConverter.ToUInt32(comInBuffer, PACKET_NO_OFFSET);

                    //if (packetNo == FINISH_PACKET_NO)
                    //{
                    //    cfg = (TRACKER_CONFIG_DATA)Lib.ByteArrayToObject(cfgData, typeof(TRACKER_CONFIG_DATA));
                    //    this.Invoke(new MethodInvoker(UpdateConfigUI));
                    //}
                    //if (packetNo == ERROR_PACKET_NO)
                    //{
                    //    return;
                    //}
                    //if (packetNo == DEFAULT_PACKET_NO)
                    //{
                    //    cfgDataSize = BitConverter.ToUInt32(comInBuffer, DATA_WITH_PACKET_NO_OFFSET);
                    //    cfgDeviceBuffSize = BitConverter.ToUInt32(comInBuffer, DATA_WITH_PACKET_NO_OFFSET + 4);
                    //    cfgDataOffset = 0;
                    //    length = (int)cfgDeviceBuffSize;
                    //}
                    //else
                    //{
                    //    Array.Copy(comInBuffer, DATA_WITH_PACKET_NO_OFFSET, cfgData, packetNo, length - PACKET_NO_SIZE);
                    //    cfgDataOffset = (int)packetNo + length - PACKET_NO_SIZE;
                    //    length = (int)cfgDeviceBuffSize;
                    //    if (cfgDataOffset > cfgDataSize - cfgDeviceBuffSize)
                    //        length = (int)cfgDataSize - cfgDataOffset;
                    //}

                    //if (cfgDataOffset >= cfgDataSize)         // last packet
                    //{
                    //    cfg = (TRACKER_CONFIG_DATA)Lib.ByteArrayToObject(cfgData, typeof(TRACKER_CONFIG_DATA));
                    //    this.Invoke(new MethodInvoker(UpdateConfigUI));
                    //    return;
                    //}
                    //CreateCOMBuffer(PACKET_NO_SIZE + 2, READ_CONFIG_OPCODE, (UInt32)cfgDataOffset);
                    //Array.Copy(BitConverter.GetBytes(length), 0, comOutBuffer, DATA_WITH_PACKET_NO_OFFSET, 2);
                    //comOutBuffer[SIMPLE_CRC_WITH_PACKET_NO_OFFSET + 2] = calcCRC(comOutBuffer, PACKET_NO_OFFSET, PACKET_NO_SIZE + 2);
                    //SendCOMData(comOutBuffer, 0, SIMPLE_PACKET_SIZE + 2);
                    break;

                case READ_FIRMWARE_VERSION_OPCODE:
                    //firmwareVersion = System.Text.Encoding.ASCII.GetString(comInBuffer, 3, length);
                    //this.Invoke((MethodInvoker)delegate
                    //{
                    //    firmwareVersionLbl.Text = firmwareVersion;
                    //});
                    break;

                case DOWNLOAD_FIRMWARE_OPCODE:
                    comTimeoutTimer.Stop();
                    if (!downloadingFirmware)
                    {
                        return;
                    }
                    packetNo = BitConverter.ToUInt32(comInBuffer, PACKET_NO_OFFSET);
                    if (packetNo == FINISH_PACKET_NO)
                    {
                        // complete
                        downloadingFirmware = false;
                        this.Invoke((MethodInvoker)delegate
                        {
                            EnableDeviceCmd(true);
                            UpdateProgressBar(false);
                        });
                        MessageBox.Show(FIRMWARE_DOWNLOADED_SUCCESS_STR.Text, APP_CAPTION_STR.Text);
                        try
                        {
                            firmwareReader.Close();
                            return;
                        }
                        catch (Exception)
                        { }
                    }
                    if (packetNo == DEFAULT_PACKET_NO)
                    {
                        fDeviceBuffSize = BitConverter.ToUInt32(comInBuffer, DATA_WITH_PACKET_NO_OFFSET + 4);
                        packetNo = 0;
                    }
                    this.Invoke((MethodInvoker)delegate
                    {
                        retrieveLogPrgBar.Value = (int)(packetNo / 100);
                        retrieveLogPercentLbl.Text = (retrieveLogPrgBar.Value * 100 / retrieveLogPrgBar.Maximum).ToString() + "%";
                    });
                    try
                    {
                        byte[] temp;

                        firmwareReader.BaseStream.Seek(packetNo, SeekOrigin.Begin);
                        //bTemp = firmwareReader.ReadBytes((int)fDeviceBuffSize);
                        temp = firmwareReader.ReadBytes((int)fDeviceBuffSize);
                        bTemp = Enumerable.Repeat<byte>(0xFF, (int)fDeviceBuffSize).ToArray();
                        Buffer.BlockCopy(temp, 0, bTemp, 0, temp.Length);
                        length = bTemp.Length + 4;
                        CreateCOMBuffer(length, DOWNLOAD_FIRMWARE_OPCODE, packetNo);
                        Array.Copy(bTemp, 0, comOutBuffer, DATA_WITH_PACKET_NO_OFFSET, bTemp.Length);
                        comOutBuffer[length + HEADER_SIZE] = calcCRC(comOutBuffer, DATA_OFFSET, length);
                        SendCOMData(comOutBuffer, 0, length + HEADER_SIZE + CRC_SIZE);
                    }
                    catch (Exception)
                    {
                        downloadingFirmware = false;
                        this.Invoke((MethodInvoker)delegate
                        {
                            EnableDeviceCmd(true);
                            UpdateProgressBar(false);
                        });
                        MessageBox.Show(FIRMWARE_DOWNLOADED_FAILED_STR.Text, APP_CAPTION_STR.Text);
                        return;
                    }
                    break;

                case SET_CAMERA_ID_OPCODE:
                    packetNo = BitConverter.ToUInt32(comInBuffer, PACKET_NO_OFFSET);
                    if (packetNo == FINISH_PACKET_NO)
                    {
                        MessageBox.Show(SET_CAMERA_ID_SUCCESS_STR.Text, APP_CAPTION_STR.Text);
                    }
                    else
                    {
                        MessageBox.Show(SET_CAMERA_ID_FAILED_STR.Text, APP_CAPTION_STR.Text);
                    }
                    break;
                case 0x5A:  // ACK
                    break;
                case 0xA5:  // NACK
                    break;
                default:
                    break;
            }
        }

        private void InitLocalizedStrings()
        {
            APP_CAPTION_STR.Name = "APP_CAPTION_STR";
            DATA_SERVER_CMBBOX_ITEM_STR.Name = "DATA_SERVER_CMBBOX_ITEM_STR";
            FIRMWARE_SERVER_CMBBOX_ITEM_STR.Name = "FIRMWARE_SERVER_CMBBOX_ITEM_STR";
            IMAGE_SERVER_CMBBOX_ITEM_STR.Name = "IMAGE_SERVER_CMBBOX_ITEM_STR";
            INTERVAL_TRIGGER_SOURCE_CHKLIST_ITEM_STR.Name = "INTERVAL_TRIGGER_SOURCE_CHKLIST_ITEM_STR";
            CONNECTED_STR.Name = "CONNECTED_STR";
            DISCONNECTED_STR.Name = "DISCONNECTED_STR";
            RETRIEVING_FILE_STR.Name = "RETRIEVING_FILE_STR";
            READ_CONFIG_SUCCESS_STR.Name = "READ_CONFIG_SUCCESS_STR";
            READ_CONFIG_FAILED_STR.Name = "READ_CONFIG_FAILED_STR";
            WRITE_CONFIG_SUCCESS_STR.Name = "WRITE_CONFIG_SUCCESS_STR";
            WRITE_CONFIG_FALIED_STR.Name = "WRITE_CONFIG_FALIED_STR";
            FORMAT_SDCARD_WARNING_STR.Name = "FORMAT_SDCARD_WARNING_STR";
            KML_SAVE_SUCCESS_STR.Name = "KML_SAVE_SUCCESS_STR";
            CONFIG_SAVE_SUCCESS_STR.Name = "CONFIG_SAVE_SUCCESS_STR";
            CONFIG_LOADED_STR.Name = "CONFIG_LOADED_STR";
            CANNOT_OPEN_COMPORT_STR.Name = "CANNOT_OPEN_COMPORT_STR";
            FIRMWARE_DOWNLOADED_SUCCESS_STR.Name = "FIRMWARE_DOWNLOADED_SUCCESS_STR";
            FIRMWARE_DOWNLOADED_FAILED_STR.Name = "FIRMWARE_DOWNLOADED_FAILED_STR";
            CANNOT_OPEN_FILE_STR.Name = "CANNOT_OPEN_FILE_STR";
            FILE_NAME_IS_INVALID_WARNING_STR.Name = "FILE_NAME_IS_INVALID_WARNING_STR";
            OPEN_FILE_DIALOG_TITLE_STR.Name = "OPEN_FILE_DIALOG_TITLE_STR";
            SAVE_FILE_DIALOG_TITLE_STR.Name = "SAVE_FILE_DIALOG_TITLE_STR";
            OPEN_FOLDER_DIALOG_TITLE_STR.Name = "OPEN_FOLDER_DIALOG_TITLE_STR";
            DOWNLOADING_FIRMWARE_STR.Name = "DOWNLOADING_FIRMWARE_STR";
            LOG_INTERVAL_1_MIN_STR.Name = "LOG_INTERVAL_1_MIN_STR";
            LOG_INTERVAL_5_MIN_STR.Name = "LOG_INTERVAL_5_MIN_STR";
            LOG_INTERVAL_10_MIN_STR.Name = "LOG_INTERVAL_10_MIN_STR";
            LOG_INTERVAL_30_MIN_STR.Name = "LOG_INTERVAL_30_MIN_STR";
            SET_CAMERA_ID_SUCCESS_STR.Name = "SET_CAMERA_ID_SUCCESS_STR";
            SET_CAMERA_ID_FAILED_STR.Name = "SET_CAMERA_ID_FAILED_STR";

            localizedStrings = new List<ResourceString>();

            localizedStrings.Add(APP_CAPTION_STR);
            localizedStrings.Add(DATA_SERVER_CMBBOX_ITEM_STR);
            localizedStrings.Add(FIRMWARE_SERVER_CMBBOX_ITEM_STR);
            localizedStrings.Add(IMAGE_SERVER_CMBBOX_ITEM_STR);
            localizedStrings.Add(INTERVAL_TRIGGER_SOURCE_CHKLIST_ITEM_STR);
            localizedStrings.Add(CONNECTED_STR);
            localizedStrings.Add(DISCONNECTED_STR);
            localizedStrings.Add(RETRIEVING_FILE_STR);
            localizedStrings.Add(READ_CONFIG_SUCCESS_STR);
            localizedStrings.Add(READ_CONFIG_FAILED_STR);
            localizedStrings.Add(WRITE_CONFIG_SUCCESS_STR);
            localizedStrings.Add(WRITE_CONFIG_FALIED_STR);
            localizedStrings.Add(FORMAT_SDCARD_WARNING_STR);
            localizedStrings.Add(KML_SAVE_SUCCESS_STR);
            localizedStrings.Add(CONFIG_SAVE_SUCCESS_STR);
            localizedStrings.Add(CONFIG_LOADED_STR);
            localizedStrings.Add(CANNOT_OPEN_COMPORT_STR);
            localizedStrings.Add(FIRMWARE_DOWNLOADED_SUCCESS_STR);
            localizedStrings.Add(FIRMWARE_DOWNLOADED_FAILED_STR);
            localizedStrings.Add(CANNOT_OPEN_FILE_STR);
            localizedStrings.Add(FILE_NAME_IS_INVALID_WARNING_STR);
            localizedStrings.Add(OPEN_FILE_DIALOG_TITLE_STR);
            localizedStrings.Add(SAVE_FILE_DIALOG_TITLE_STR);
            localizedStrings.Add(OPEN_FOLDER_DIALOG_TITLE_STR);
            localizedStrings.Add(DOWNLOADING_FIRMWARE_STR);
            localizedStrings.Add(LOG_INTERVAL_1_MIN_STR);
            localizedStrings.Add(LOG_INTERVAL_5_MIN_STR);
            localizedStrings.Add(LOG_INTERVAL_10_MIN_STR);
            localizedStrings.Add(LOG_INTERVAL_30_MIN_STR);
            localizedStrings.Add(SET_CAMERA_ID_SUCCESS_STR);
            localizedStrings.Add(SET_CAMERA_ID_FAILED_STR);

            //ChangeLanguage("vi");
        }

        private void LoadLocalizedString(string lang)
        {
            //ResourceManager rm;

            //rm = NgonNgu.Resources.ViStrings.ResourceManager;
            

            //for (int i = 0; i < localizedStrings.Count; i++)
            //{
            //    localizedStrings[i].Text = rm.GetString(localizedStrings[i].Name);
            //}

            //serverSelectionCmbBox.Items[0] = DATA_SERVER_CMBBOX_ITEM_STR.Text;
            ////serverSelectionCmbBox.Items[1] = FIRMWARE_SERVER_CMBBOX_ITEM_STR.Text;
            //serverSelectionCmbBox.Items[1] = IMAGE_SERVER_CMBBOX_ITEM_STR.Text;
            //triggerSourceChkList.Items[0] = INTERVAL_TRIGGER_SOURCE_CHKLIST_ITEM_STR.Text;
            //logIntervalCmbBox.Items[4] = LOG_INTERVAL_1_MIN_STR.Text;
            //logIntervalCmbBox.Items[5] = LOG_INTERVAL_5_MIN_STR.Text;
            //logIntervalCmbBox.Items[6] = LOG_INTERVAL_10_MIN_STR.Text;
            //logIntervalCmbBox.Items[7] = LOG_INTERVAL_30_MIN_STR.Text;
            //openFileDialog1.Title = APP_CAPTION_STR.Text + " - " + OPEN_FILE_DIALOG_TITLE_STR.Text;
            //saveFileDialog1.Title = APP_CAPTION_STR.Text + " - " + SAVE_FILE_DIALOG_TITLE_STR.Text;
            //selectFolderDialog.Description = APP_CAPTION_STR.Text + " - " + OPEN_FOLDER_DIALOG_TITLE_STR.Text;
        }

        private void button3_Click(object sender, EventArgs e)
        {

        }

        private void button4_Click(object sender, EventArgs e)
        {

        }
    }


    public class ResourceString
    {
        string name;
        string text;

        public string Name
        {
            get { return name; }
            set { name = value; }
        }

        public string Text
        {
            get { return text; }
            set { text = value; }
        }
    }
}
