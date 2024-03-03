namespace CANguruX
{
    partial class Form1
    {
        /// <summary>
        /// Erforderliche Designervariable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Verwendete Ressourcen bereinigen.
        /// </summary>
        /// <param name="disposing">True, wenn verwaltete Ressourcen gelöscht werden sollen; andernfalls False.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Vom Windows Form-Designer generierter Code

        /// <summary>
        /// Erforderliche Methode für die Designerunterstützung.
        /// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.beenden = new System.Windows.Forms.Button();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.Telnet = new System.Windows.Forms.TabPage();
            this.groupCommand = new System.Windows.Forms.GroupBox();
            this.progressBarPing = new System.Windows.Forms.ProgressBar();
            this.TelnetComm = new System.Windows.Forms.TextBox();
            this.groupInput = new System.Windows.Forms.GroupBox();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.timeBox = new System.Windows.Forms.TextBox();
            this.tbConnectAdr = new System.Windows.Forms.TextBox();
            this.groupAction = new System.Windows.Forms.GroupBox();
            this.buttonConnect = new System.Windows.Forms.Button();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.MFX = new System.Windows.Forms.TabPage();
            this.groupBox7 = new System.Windows.Forms.GroupBox();
            this.writeName = new System.Windows.Forms.Button();
            this.label12 = new System.Windows.Forms.Label();
            this.readName = new System.Windows.Forms.Button();
            this.txtBxLokname = new System.Windows.Forms.TextBox();
            this.groupBox6 = new System.Windows.Forms.GroupBox();
            this.newCV = new System.Windows.Forms.NumericUpDown();
            this.CVresult = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.CVwrite = new System.Windows.Forms.Button();
            this.label5 = new System.Windows.Forms.Label();
            this.CVread = new System.Windows.Forms.Button();
            this.oldCV = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.CV = new System.Windows.Forms.NumericUpDown();
            this.label1 = new System.Windows.Forms.Label();
            this.progressBarMfx = new System.Windows.Forms.ProgressBar();
            this.numLokAdress = new System.Windows.Forms.NumericUpDown();
            this.lstBoxDecoderType = new System.Windows.Forms.ListBox();
            this.label10 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.txtBoxLokName = new System.Windows.Forms.TextBox();
            this.genLokListmfx = new System.Windows.Forms.Button();
            this.delLok = new System.Windows.Forms.Button();
            this.findLoks = new System.Windows.Forms.Button();
            this.lokBox = new System.Windows.Forms.ListBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.mfxProgress = new System.Windows.Forms.TextBox();
            this.numCounter = new System.Windows.Forms.NumericUpDown();
            this.mfxDiscovery = new System.Windows.Forms.Button();
            this.numLocID = new System.Windows.Forms.NumericUpDown();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.getLok = new System.Windows.Forms.Button();
            this.label11 = new System.Windows.Forms.Label();
            this.Configuration = new System.Windows.Forms.TabPage();
            this.fileSaveButton = new System.Windows.Forms.Button();
            this.resetButton = new System.Windows.Forms.Button();
            this.deviceIP = new System.Windows.Forms.TextBox();
            this.otaBtn = new System.Windows.Forms.Button();
            this.CANElemente = new System.Windows.Forms.ListBox();
            this.btnVolt = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label7 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.numUpDnDecNumber = new System.Windows.Forms.NumericUpDown();
            this.numUpDnDelay = new System.Windows.Forms.NumericUpDown();
            this.btnGetData = new System.Windows.Forms.Button();
            this.btnSetData = new System.Windows.Forms.Button();
            this.btnVerbose = new System.Windows.Forms.Button();
            this.tabControl1.SuspendLayout();
            this.Telnet.SuspendLayout();
            this.groupCommand.SuspendLayout();
            this.groupInput.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.groupAction.SuspendLayout();
            this.MFX.SuspendLayout();
            this.groupBox7.SuspendLayout();
            this.groupBox6.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.newCV)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.CV)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numLokAdress)).BeginInit();
            this.groupBox2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numCounter)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numLocID)).BeginInit();
            this.groupBox3.SuspendLayout();
            this.Configuration.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDnDecNumber)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDnDelay)).BeginInit();
            this.SuspendLayout();
            // 
            // beenden
            // 
            this.beenden.Location = new System.Drawing.Point(384, 548);
            this.beenden.Name = "beenden";
            this.beenden.Size = new System.Drawing.Size(90, 25);
            this.beenden.TabIndex = 5;
            this.beenden.Text = "Beenden";
            this.beenden.UseVisualStyleBackColor = true;
            this.beenden.Click += new System.EventHandler(this.beenden_Click);
            // 
            // tabControl1
            // 
            this.tabControl1.Alignment = System.Windows.Forms.TabAlignment.Bottom;
            this.tabControl1.Controls.Add(this.Telnet);
            this.tabControl1.Controls.Add(this.MFX);
            this.tabControl1.Controls.Add(this.Configuration);
            this.tabControl1.Location = new System.Drawing.Point(-1, 2);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(487, 542);
            this.tabControl1.TabIndex = 12;
            this.tabControl1.SelectedIndexChanged += new System.EventHandler(this.TabControl1_SelectedIndexChanged);
            // 
            // Telnet
            // 
            this.Telnet.Controls.Add(this.groupCommand);
            this.Telnet.Controls.Add(this.groupInput);
            this.Telnet.Location = new System.Drawing.Point(4, 4);
            this.Telnet.Name = "Telnet";
            this.Telnet.Padding = new System.Windows.Forms.Padding(3);
            this.Telnet.Size = new System.Drawing.Size(479, 516);
            this.Telnet.TabIndex = 1;
            this.Telnet.Text = "Telnet";
            this.Telnet.UseVisualStyleBackColor = true;
            // 
            // groupCommand
            // 
            this.groupCommand.Controls.Add(this.progressBarPing);
            this.groupCommand.Controls.Add(this.TelnetComm);
            this.groupCommand.Location = new System.Drawing.Point(8, 86);
            this.groupCommand.Margin = new System.Windows.Forms.Padding(2);
            this.groupCommand.Name = "groupCommand";
            this.groupCommand.Padding = new System.Windows.Forms.Padding(2);
            this.groupCommand.Size = new System.Drawing.Size(463, 420);
            this.groupCommand.TabIndex = 9;
            this.groupCommand.TabStop = false;
            this.groupCommand.Text = "Command";
            // 
            // progressBarPing
            // 
            this.progressBarPing.Location = new System.Drawing.Point(5, 189);
            this.progressBarPing.Name = "progressBarPing";
            this.progressBarPing.Size = new System.Drawing.Size(449, 23);
            this.progressBarPing.TabIndex = 6;
            // 
            // TelnetComm
            // 
            this.TelnetComm.AcceptsReturn = true;
            this.TelnetComm.Font = new System.Drawing.Font("Courier New", 8.5F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TelnetComm.Location = new System.Drawing.Point(5, 20);
            this.TelnetComm.Multiline = true;
            this.TelnetComm.Name = "TelnetComm";
            this.TelnetComm.Size = new System.Drawing.Size(450, 395);
            this.TelnetComm.TabIndex = 7;
            // 
            // groupInput
            // 
            this.groupInput.Controls.Add(this.groupBox5);
            this.groupInput.Controls.Add(this.tbConnectAdr);
            this.groupInput.Controls.Add(this.groupAction);
            this.groupInput.Controls.Add(this.groupBox4);
            this.groupInput.Location = new System.Drawing.Point(8, 8);
            this.groupInput.Margin = new System.Windows.Forms.Padding(2);
            this.groupInput.Name = "groupInput";
            this.groupInput.Padding = new System.Windows.Forms.Padding(2);
            this.groupInput.Size = new System.Drawing.Size(463, 81);
            this.groupInput.TabIndex = 7;
            this.groupInput.TabStop = false;
            this.groupInput.Text = "Connect Information";
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.timeBox);
            this.groupBox5.Location = new System.Drawing.Point(151, 20);
            this.groupBox5.Margin = new System.Windows.Forms.Padding(2);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Padding = new System.Windows.Forms.Padding(2);
            this.groupBox5.Size = new System.Drawing.Size(115, 57);
            this.groupBox5.TabIndex = 7;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "Time";
            // 
            // timeBox
            // 
            this.timeBox.Location = new System.Drawing.Point(8, 18);
            this.timeBox.Multiline = true;
            this.timeBox.Name = "timeBox";
            this.timeBox.Size = new System.Drawing.Size(100, 34);
            this.timeBox.TabIndex = 0;
            this.timeBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // tbConnectAdr
            // 
            this.tbConnectAdr.Location = new System.Drawing.Point(25, 47);
            this.tbConnectAdr.Name = "tbConnectAdr";
            this.tbConnectAdr.ReadOnly = true;
            this.tbConnectAdr.Size = new System.Drawing.Size(100, 20);
            this.tbConnectAdr.TabIndex = 5;
            this.tbConnectAdr.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // groupAction
            // 
            this.groupAction.Controls.Add(this.buttonConnect);
            this.groupAction.Location = new System.Drawing.Point(273, 20);
            this.groupAction.Margin = new System.Windows.Forms.Padding(2);
            this.groupAction.Name = "groupAction";
            this.groupAction.Padding = new System.Windows.Forms.Padding(2);
            this.groupAction.Size = new System.Drawing.Size(186, 57);
            this.groupAction.TabIndex = 3;
            this.groupAction.TabStop = false;
            this.groupAction.Text = "Action";
            // 
            // buttonConnect
            // 
            this.buttonConnect.Location = new System.Drawing.Point(15, 23);
            this.buttonConnect.Margin = new System.Windows.Forms.Padding(2);
            this.buttonConnect.Name = "buttonConnect";
            this.buttonConnect.Size = new System.Drawing.Size(161, 25);
            this.buttonConnect.TabIndex = 0;
            this.buttonConnect.Text = "Connect!";
            this.buttonConnect.UseVisualStyleBackColor = true;
            this.buttonConnect.Click += new System.EventHandler(this.onConnectClick);
            // 
            // groupBox4
            // 
            this.groupBox4.Location = new System.Drawing.Point(5, 20);
            this.groupBox4.Margin = new System.Windows.Forms.Padding(2);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Padding = new System.Windows.Forms.Padding(2);
            this.groupBox4.Size = new System.Drawing.Size(139, 57);
            this.groupBox4.TabIndex = 6;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "CANguru-Bridge Address";
            // 
            // MFX
            // 
            this.MFX.Controls.Add(this.groupBox7);
            this.MFX.Controls.Add(this.groupBox6);
            this.MFX.Controls.Add(this.progressBarMfx);
            this.MFX.Controls.Add(this.numLokAdress);
            this.MFX.Controls.Add(this.lstBoxDecoderType);
            this.MFX.Controls.Add(this.label10);
            this.MFX.Controls.Add(this.label9);
            this.MFX.Controls.Add(this.txtBoxLokName);
            this.MFX.Controls.Add(this.genLokListmfx);
            this.MFX.Controls.Add(this.delLok);
            this.MFX.Controls.Add(this.findLoks);
            this.MFX.Controls.Add(this.lokBox);
            this.MFX.Controls.Add(this.groupBox2);
            this.MFX.Controls.Add(this.groupBox3);
            this.MFX.Location = new System.Drawing.Point(4, 4);
            this.MFX.Name = "MFX";
            this.MFX.Padding = new System.Windows.Forms.Padding(3);
            this.MFX.Size = new System.Drawing.Size(479, 516);
            this.MFX.TabIndex = 0;
            this.MFX.Text = "MFX-Ctrl";
            this.MFX.UseVisualStyleBackColor = true;
            // 
            // groupBox7
            // 
            this.groupBox7.Controls.Add(this.writeName);
            this.groupBox7.Controls.Add(this.label12);
            this.groupBox7.Controls.Add(this.readName);
            this.groupBox7.Controls.Add(this.txtBxLokname);
            this.groupBox7.Location = new System.Drawing.Point(10, 454);
            this.groupBox7.Name = "groupBox7";
            this.groupBox7.Size = new System.Drawing.Size(463, 56);
            this.groupBox7.TabIndex = 39;
            this.groupBox7.TabStop = false;
            this.groupBox7.Text = "Lokname (nur mfx-loks)";
            // 
            // writeName
            // 
            this.writeName.Location = new System.Drawing.Point(361, 22);
            this.writeName.Name = "writeName";
            this.writeName.Size = new System.Drawing.Size(90, 23);
            this.writeName.TabIndex = 14;
            this.writeName.Text = "Schreiben";
            this.writeName.UseVisualStyleBackColor = true;
            this.writeName.Click += new System.EventHandler(this.writeName_Click);
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(16, 27);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(51, 13);
            this.label12.TabIndex = 11;
            this.label12.Text = "Lokname";
            // 
            // readName
            // 
            this.readName.Location = new System.Drawing.Point(251, 23);
            this.readName.Name = "readName";
            this.readName.Size = new System.Drawing.Size(90, 23);
            this.readName.TabIndex = 13;
            this.readName.Text = "Lesen";
            this.readName.UseVisualStyleBackColor = true;
            this.readName.Click += new System.EventHandler(this.readName_Click);
            // 
            // txtBxLokname
            // 
            this.txtBxLokname.Location = new System.Drawing.Point(84, 25);
            this.txtBxLokname.Name = "txtBxLokname";
            this.txtBxLokname.Size = new System.Drawing.Size(139, 20);
            this.txtBxLokname.TabIndex = 12;
            // 
            // groupBox6
            // 
            this.groupBox6.Controls.Add(this.newCV);
            this.groupBox6.Controls.Add(this.CVresult);
            this.groupBox6.Controls.Add(this.label6);
            this.groupBox6.Controls.Add(this.CVwrite);
            this.groupBox6.Controls.Add(this.label5);
            this.groupBox6.Controls.Add(this.CVread);
            this.groupBox6.Controls.Add(this.oldCV);
            this.groupBox6.Controls.Add(this.label4);
            this.groupBox6.Controls.Add(this.CV);
            this.groupBox6.Controls.Add(this.label1);
            this.groupBox6.Location = new System.Drawing.Point(6, 357);
            this.groupBox6.Name = "groupBox6";
            this.groupBox6.Size = new System.Drawing.Size(467, 90);
            this.groupBox6.TabIndex = 38;
            this.groupBox6.TabStop = false;
            this.groupBox6.Text = "CV lesen / schreiben";
            // 
            // newCV
            // 
            this.newCV.Location = new System.Drawing.Point(389, 53);
            this.newCV.Maximum = new decimal(new int[] {
            255,
            0,
            0,
            0});
            this.newCV.Name = "newCV";
            this.newCV.Size = new System.Drawing.Size(72, 20);
            this.newCV.TabIndex = 10;
            // 
            // CVresult
            // 
            this.CVresult.Location = new System.Drawing.Point(299, 20);
            this.CVresult.Name = "CVresult";
            this.CVresult.Size = new System.Drawing.Size(75, 20);
            this.CVresult.TabIndex = 9;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(240, 23);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(48, 13);
            this.label6.TabIndex = 8;
            this.label6.Text = "Ergebnis";
            // 
            // CVwrite
            // 
            this.CVwrite.Location = new System.Drawing.Point(299, 53);
            this.CVwrite.Name = "CVwrite";
            this.CVwrite.Size = new System.Drawing.Size(75, 23);
            this.CVwrite.TabIndex = 6;
            this.CVwrite.Text = "Schreiben";
            this.CVwrite.UseVisualStyleBackColor = true;
            this.CVwrite.Click += new System.EventHandler(this.CVwrite_Click);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(231, 58);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(62, 13);
            this.label5.TabIndex = 5;
            this.label5.Text = "Neuer Wert";
            // 
            // CVread
            // 
            this.CVread.Location = new System.Drawing.Point(81, 53);
            this.CVread.Name = "CVread";
            this.CVread.Size = new System.Drawing.Size(75, 23);
            this.CVread.TabIndex = 4;
            this.CVread.Text = "Lesen";
            this.CVread.UseVisualStyleBackColor = true;
            this.CVread.Click += new System.EventHandler(this.CVread_Click);
            // 
            // oldCV
            // 
            this.oldCV.Location = new System.Drawing.Point(162, 55);
            this.oldCV.Name = "oldCV";
            this.oldCV.ReadOnly = true;
            this.oldCV.Size = new System.Drawing.Size(50, 20);
            this.oldCV.TabIndex = 3;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(21, 58);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(54, 13);
            this.label4.TabIndex = 2;
            this.label4.Text = "Alter Wert";
            // 
            // CV
            // 
            this.CV.Location = new System.Drawing.Point(81, 22);
            this.CV.Maximum = new decimal(new int[] {
            1023,
            0,
            0,
            0});
            this.CV.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.CV.Name = "CV";
            this.CV.Size = new System.Drawing.Size(96, 20);
            this.CV.TabIndex = 1;
            this.CV.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(20, 25);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(62, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "CV-Adresse";
            // 
            // progressBarMfx
            // 
            this.progressBarMfx.Location = new System.Drawing.Point(15, 275);
            this.progressBarMfx.Name = "progressBarMfx";
            this.progressBarMfx.Size = new System.Drawing.Size(449, 23);
            this.progressBarMfx.TabIndex = 34;
            // 
            // numLokAdress
            // 
            this.numLokAdress.Location = new System.Drawing.Point(133, 178);
            this.numLokAdress.Maximum = new decimal(new int[] {
            255,
            0,
            0,
            0});
            this.numLokAdress.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numLokAdress.Name = "numLokAdress";
            this.numLokAdress.Size = new System.Drawing.Size(100, 20);
            this.numLokAdress.TabIndex = 33;
            this.numLokAdress.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            // 
            // lstBoxDecoderType
            // 
            this.lstBoxDecoderType.FormattingEnabled = true;
            this.lstBoxDecoderType.Items.AddRange(new object[] {
            "MM2_DIL8",
            "MM1_PRG",
            "DCC"});
            this.lstBoxDecoderType.Location = new System.Drawing.Point(133, 212);
            this.lstBoxDecoderType.Name = "lstBoxDecoderType";
            this.lstBoxDecoderType.Size = new System.Drawing.Size(120, 43);
            this.lstBoxDecoderType.TabIndex = 31;
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(27, 186);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(45, 13);
            this.label10.TabIndex = 28;
            this.label10.Text = "Adresse";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(27, 154);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(35, 13);
            this.label9.TabIndex = 26;
            this.label9.Text = "Name";
            // 
            // txtBoxLokName
            // 
            this.txtBoxLokName.Location = new System.Drawing.Point(133, 148);
            this.txtBoxLokName.Name = "txtBoxLokName";
            this.txtBoxLokName.Size = new System.Drawing.Size(100, 20);
            this.txtBoxLokName.TabIndex = 25;
            // 
            // genLokListmfx
            // 
            this.genLokListmfx.Location = new System.Drawing.Point(315, 97);
            this.genLokListmfx.Name = "genLokListmfx";
            this.genLokListmfx.Size = new System.Drawing.Size(120, 25);
            this.genLokListmfx.TabIndex = 24;
            this.genLokListmfx.Text = "Erzeuge Lok-Liste";
            this.genLokListmfx.UseVisualStyleBackColor = true;
            this.genLokListmfx.Click += new System.EventHandler(this.genLokListmfx_Click);
            // 
            // delLok
            // 
            this.delLok.Location = new System.Drawing.Point(162, 97);
            this.delLok.Name = "delLok";
            this.delLok.Size = new System.Drawing.Size(120, 25);
            this.delLok.TabIndex = 23;
            this.delLok.Text = "Lösche Lok";
            this.delLok.UseVisualStyleBackColor = true;
            this.delLok.Click += new System.EventHandler(this.delLok_Click);
            // 
            // findLoks
            // 
            this.findLoks.Location = new System.Drawing.Point(4, 97);
            this.findLoks.Name = "findLoks";
            this.findLoks.Size = new System.Drawing.Size(120, 25);
            this.findLoks.TabIndex = 20;
            this.findLoks.Text = "Liste Loks";
            this.findLoks.UseVisualStyleBackColor = true;
            this.findLoks.Click += new System.EventHandler(this.findLoks_Click);
            // 
            // lokBox
            // 
            this.lokBox.FormattingEnabled = true;
            this.lokBox.Location = new System.Drawing.Point(5, 9);
            this.lokBox.Name = "lokBox";
            this.lokBox.Size = new System.Drawing.Size(430, 82);
            this.lokBox.TabIndex = 19;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.mfxProgress);
            this.groupBox2.Controls.Add(this.numCounter);
            this.groupBox2.Controls.Add(this.mfxDiscovery);
            this.groupBox2.Controls.Add(this.numLocID);
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Location = new System.Drawing.Point(5, 275);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(468, 76);
            this.groupBox2.TabIndex = 36;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "MFX-Loks anmelden";
            // 
            // mfxProgress
            // 
            this.mfxProgress.Location = new System.Drawing.Point(178, 18);
            this.mfxProgress.Name = "mfxProgress";
            this.mfxProgress.Size = new System.Drawing.Size(120, 20);
            this.mfxProgress.TabIndex = 23;
            // 
            // numCounter
            // 
            this.numCounter.Location = new System.Drawing.Point(128, 19);
            this.numCounter.Name = "numCounter";
            this.numCounter.Size = new System.Drawing.Size(40, 20);
            this.numCounter.TabIndex = 21;
            this.numCounter.Click += new System.EventHandler(this.numCounter_ValueChanged);
            // 
            // mfxDiscovery
            // 
            this.mfxDiscovery.Location = new System.Drawing.Point(310, 18);
            this.mfxDiscovery.Name = "mfxDiscovery";
            this.mfxDiscovery.Size = new System.Drawing.Size(120, 21);
            this.mfxDiscovery.TabIndex = 12;
            this.mfxDiscovery.Text = "MFX-Lok erfassen";
            this.mfxDiscovery.UseVisualStyleBackColor = true;
            this.mfxDiscovery.Click += new System.EventHandler(this.mfxLoks_Click);
            // 
            // numLocID
            // 
            this.numLocID.Location = new System.Drawing.Point(128, 44);
            this.numLocID.Name = "numLocID";
            this.numLocID.Size = new System.Drawing.Size(40, 20);
            this.numLocID.TabIndex = 22;
            this.numLocID.Click += new System.EventHandler(this.numLocID_ValueChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(22, 21);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(95, 13);
            this.label2.TabIndex = 15;
            this.label2.Text = "Neuanmeldezähler";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(22, 46);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(89, 13);
            this.label3.TabIndex = 14;
            this.label3.Text = "Schienenadresse";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.getLok);
            this.groupBox3.Controls.Add(this.label11);
            this.groupBox3.Location = new System.Drawing.Point(5, 130);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(468, 139);
            this.groupBox3.TabIndex = 37;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Digital-Loks anmelden";
            // 
            // getLok
            // 
            this.getLok.Location = new System.Drawing.Point(310, 95);
            this.getLok.Name = "getLok";
            this.getLok.Size = new System.Drawing.Size(120, 25);
            this.getLok.TabIndex = 32;
            this.getLok.Text = "Digital-Lok erfassen";
            this.getLok.UseVisualStyleBackColor = true;
            this.getLok.Click += new System.EventHandler(this.getLok_Click);
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(22, 99);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(62, 13);
            this.label11.TabIndex = 30;
            this.label11.Text = "Dekodertyp";
            // 
            // Configuration
            // 
            this.Configuration.AutoScroll = true;
            this.Configuration.Controls.Add(this.fileSaveButton);
            this.Configuration.Controls.Add(this.resetButton);
            this.Configuration.Controls.Add(this.deviceIP);
            this.Configuration.Controls.Add(this.otaBtn);
            this.Configuration.Controls.Add(this.CANElemente);
            this.Configuration.Location = new System.Drawing.Point(4, 4);
            this.Configuration.Name = "Configuration";
            this.Configuration.Size = new System.Drawing.Size(479, 516);
            this.Configuration.TabIndex = 3;
            this.Configuration.Text = "Konfiguration";
            this.Configuration.UseVisualStyleBackColor = true;
            // 
            // fileSaveButton
            // 
            this.fileSaveButton.Location = new System.Drawing.Point(292, 77);
            this.fileSaveButton.Name = "fileSaveButton";
            this.fileSaveButton.Size = new System.Drawing.Size(135, 23);
            this.fileSaveButton.TabIndex = 6;
            this.fileSaveButton.Text = "Decoderdaten speichern";
            this.fileSaveButton.UseVisualStyleBackColor = true;
            this.fileSaveButton.Click += new System.EventHandler(this.fileSaveButton_Click);
            // 
            // resetButton
            // 
            this.resetButton.Location = new System.Drawing.Point(292, 48);
            this.resetButton.Name = "resetButton";
            this.resetButton.Size = new System.Drawing.Size(135, 23);
            this.resetButton.TabIndex = 5;
            this.resetButton.Text = "Reset Decoder";
            this.resetButton.UseVisualStyleBackColor = true;
            this.resetButton.Click += new System.EventHandler(this.resetButton_Click);
            // 
            // deviceIP
            // 
            this.deviceIP.Location = new System.Drawing.Point(301, 112);
            this.deviceIP.Name = "deviceIP";
            this.deviceIP.Size = new System.Drawing.Size(100, 20);
            this.deviceIP.TabIndex = 4;
            // 
            // otaBtn
            // 
            this.otaBtn.Location = new System.Drawing.Point(292, 19);
            this.otaBtn.Name = "otaBtn";
            this.otaBtn.Size = new System.Drawing.Size(135, 23);
            this.otaBtn.TabIndex = 3;
            this.otaBtn.Text = "Over The Air (OTA)";
            this.otaBtn.UseVisualStyleBackColor = true;
            this.otaBtn.Click += new System.EventHandler(this.otaBtn_Click);
            // 
            // CANElemente
            // 
            this.CANElemente.FormattingEnabled = true;
            this.CANElemente.Location = new System.Drawing.Point(50, 27);
            this.CANElemente.Name = "CANElemente";
            this.CANElemente.Size = new System.Drawing.Size(182, 108);
            this.CANElemente.TabIndex = 1;
            this.CANElemente.SelectedIndexChanged += new System.EventHandler(this.CANElemente_SelectedIndexChanged);
            // 
            // btnVolt
            // 
            this.btnVolt.Location = new System.Drawing.Point(29, 550);
            this.btnVolt.Name = "btnVolt";
            this.btnVolt.Size = new System.Drawing.Size(134, 23);
            this.btnVolt.TabIndex = 20;
            this.btnVolt.Text = "Gleisspannung AUS";
            this.btnVolt.UseVisualStyleBackColor = true;
            this.btnVolt.Click += new System.EventHandler(this.btnVolt_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(9, 6);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(449, 360);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            // 
            // label7
            // 
            this.label7.Location = new System.Drawing.Point(0, 0);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(100, 23);
            this.label7.TabIndex = 0;
            // 
            // label8
            // 
            this.label8.Location = new System.Drawing.Point(0, 0);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(100, 23);
            this.label8.TabIndex = 0;
            // 
            // numUpDnDecNumber
            // 
            this.numUpDnDecNumber.Location = new System.Drawing.Point(0, 0);
            this.numUpDnDecNumber.Name = "numUpDnDecNumber";
            this.numUpDnDecNumber.Size = new System.Drawing.Size(120, 20);
            this.numUpDnDecNumber.TabIndex = 0;
            // 
            // numUpDnDelay
            // 
            this.numUpDnDelay.Location = new System.Drawing.Point(0, 0);
            this.numUpDnDelay.Name = "numUpDnDelay";
            this.numUpDnDelay.Size = new System.Drawing.Size(120, 20);
            this.numUpDnDelay.TabIndex = 0;
            // 
            // btnGetData
            // 
            this.btnGetData.Location = new System.Drawing.Point(0, 0);
            this.btnGetData.Name = "btnGetData";
            this.btnGetData.Size = new System.Drawing.Size(75, 23);
            this.btnGetData.TabIndex = 0;
            // 
            // btnSetData
            // 
            this.btnSetData.Location = new System.Drawing.Point(0, 0);
            this.btnSetData.Name = "btnSetData";
            this.btnSetData.Size = new System.Drawing.Size(75, 23);
            this.btnSetData.TabIndex = 0;
            // 
            // btnVerbose
            // 
            this.btnVerbose.Location = new System.Drawing.Point(202, 549);
            this.btnVerbose.Name = "btnVerbose";
            this.btnVerbose.Size = new System.Drawing.Size(134, 23);
            this.btnVerbose.TabIndex = 21;
            this.btnVerbose.Text = "btnVerbose";
            this.btnVerbose.UseVisualStyleBackColor = true;
            this.btnVerbose.Click += new System.EventHandler(this.btnVerbose_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(493, 579);
            this.Controls.Add(this.btnVerbose);
            this.Controls.Add(this.btnVolt);
            this.Controls.Add(this.beenden);
            this.Controls.Add(this.tabControl1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "Form1";
            this.Text = "CANguru-Server 3.50";
            this.tabControl1.ResumeLayout(false);
            this.Telnet.ResumeLayout(false);
            this.groupCommand.ResumeLayout(false);
            this.groupCommand.PerformLayout();
            this.groupInput.ResumeLayout(false);
            this.groupInput.PerformLayout();
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.groupAction.ResumeLayout(false);
            this.MFX.ResumeLayout(false);
            this.MFX.PerformLayout();
            this.groupBox7.ResumeLayout(false);
            this.groupBox7.PerformLayout();
            this.groupBox6.ResumeLayout(false);
            this.groupBox6.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.newCV)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.CV)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numLokAdress)).EndInit();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numCounter)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numLocID)).EndInit();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.Configuration.ResumeLayout(false);
            this.Configuration.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDnDecNumber)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDnDelay)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.Button beenden;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage MFX;
        private System.Windows.Forms.Button genLokListmfx;
        private System.Windows.Forms.Button delLok;
        private System.Windows.Forms.NumericUpDown numLocID;
        private System.Windows.Forms.NumericUpDown numCounter;
        private System.Windows.Forms.Button findLoks;
        private System.Windows.Forms.ListBox lokBox;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button mfxDiscovery;
        private System.Windows.Forms.TabPage Telnet;
        private System.Windows.Forms.GroupBox groupCommand;
        private System.Windows.Forms.ProgressBar progressBarPing;
        private System.Windows.Forms.ProgressBar progressBarMfx;
        private System.Windows.Forms.TextBox TelnetComm;
        private System.Windows.Forms.GroupBox groupInput;
        private System.Windows.Forms.Button buttonConnect;
        private System.Windows.Forms.GroupBox groupAction;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.TextBox txtBoxLokName;
        private System.Windows.Forms.Button getLok;
        private System.Windows.Forms.ListBox lstBoxDecoderType;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.NumericUpDown numLokAdress;
        private System.Windows.Forms.Button btnVolt;
        private System.Windows.Forms.TabPage Configuration;
        private System.Windows.Forms.ListBox CANElemente;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.NumericUpDown numUpDnDecNumber;
        private System.Windows.Forms.NumericUpDown numUpDnDelay;
        private System.Windows.Forms.Button btnGetData;
        private System.Windows.Forms.Button btnSetData;
        private System.Windows.Forms.Button otaBtn;
        private System.Windows.Forms.TextBox deviceIP;
        private System.Windows.Forms.TextBox tbConnectAdr;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.Button btnVerbose;
        private System.Windows.Forms.Button resetButton;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.GroupBox groupBox5;
        private System.Windows.Forms.TextBox timeBox;
        private System.Windows.Forms.TextBox mfxProgress;
        private System.Windows.Forms.GroupBox groupBox6;
        private System.Windows.Forms.TextBox CVresult;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Button CVwrite;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Button CVread;
        private System.Windows.Forms.TextBox oldCV;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.NumericUpDown CV;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.NumericUpDown newCV;
        private System.Windows.Forms.Button readName;
        private System.Windows.Forms.TextBox txtBxLokname;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.GroupBox groupBox7;
        private System.Windows.Forms.Button writeName;
        private System.Windows.Forms.Button fileSaveButton;
    }
}

