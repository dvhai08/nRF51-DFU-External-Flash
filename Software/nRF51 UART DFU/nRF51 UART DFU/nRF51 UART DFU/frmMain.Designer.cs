namespace nRF51_UART_DFU
{
    partial class frmMain
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.firmwarePathTxtBox = new System.Windows.Forms.TextBox();
            this.btnOpen = new System.Windows.Forms.Button();
            this.btnDownload = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.button4 = new System.Windows.Forms.Button();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.retrieveLogPrgBar = new System.Windows.Forms.ProgressBar();
            this.retrieveLogStatusLbl = new System.Windows.Forms.Label();
            this.btnOpenPort = new System.Windows.Forms.Button();
            this.comPortCmbBox = new System.Windows.Forms.ComboBox();
            this.retrieveLogPercentLbl = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // firmwarePathTxtBox
            // 
            this.firmwarePathTxtBox.Location = new System.Drawing.Point(12, 28);
            this.firmwarePathTxtBox.Name = "firmwarePathTxtBox";
            this.firmwarePathTxtBox.Size = new System.Drawing.Size(402, 20);
            this.firmwarePathTxtBox.TabIndex = 0;
            // 
            // btnOpen
            // 
            this.btnOpen.Location = new System.Drawing.Point(420, 25);
            this.btnOpen.Name = "btnOpen";
            this.btnOpen.Size = new System.Drawing.Size(73, 23);
            this.btnOpen.TabIndex = 1;
            this.btnOpen.Text = "Open";
            this.btnOpen.UseVisualStyleBackColor = true;
            this.btnOpen.Click += new System.EventHandler(this.btnOpen_Click);
            // 
            // btnDownload
            // 
            this.btnDownload.Location = new System.Drawing.Point(336, 66);
            this.btnDownload.Name = "btnDownload";
            this.btnDownload.Size = new System.Drawing.Size(73, 23);
            this.btnDownload.TabIndex = 1;
            this.btnDownload.Text = "Download";
            this.btnDownload.UseVisualStyleBackColor = true;
            this.btnDownload.Click += new System.EventHandler(this.btnDownload_Click);
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(257, 66);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(73, 23);
            this.button3.TabIndex = 1;
            this.button3.Text = "Read";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.button3_Click);
            // 
            // button4
            // 
            this.button4.Location = new System.Drawing.Point(420, 66);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(73, 23);
            this.button4.TabIndex = 2;
            this.button4.Text = "Run App";
            this.button4.UseVisualStyleBackColor = true;
            this.button4.Click += new System.EventHandler(this.button4_Click);
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.FileName = "openFileDialog1";
            // 
            // retrieveLogPrgBar
            // 
            this.retrieveLogPrgBar.Location = new System.Drawing.Point(12, 118);
            this.retrieveLogPrgBar.Name = "retrieveLogPrgBar";
            this.retrieveLogPrgBar.Size = new System.Drawing.Size(402, 23);
            this.retrieveLogPrgBar.TabIndex = 3;
            // 
            // retrieveLogStatusLbl
            // 
            this.retrieveLogStatusLbl.AutoSize = true;
            this.retrieveLogStatusLbl.Location = new System.Drawing.Point(12, 159);
            this.retrieveLogStatusLbl.Name = "retrieveLogStatusLbl";
            this.retrieveLogStatusLbl.Size = new System.Drawing.Size(35, 13);
            this.retrieveLogStatusLbl.TabIndex = 4;
            this.retrieveLogStatusLbl.Text = "label1";
            // 
            // btnOpenPort
            // 
            this.btnOpenPort.Location = new System.Drawing.Point(420, 168);
            this.btnOpenPort.Name = "btnOpenPort";
            this.btnOpenPort.Size = new System.Drawing.Size(73, 23);
            this.btnOpenPort.TabIndex = 5;
            this.btnOpenPort.Text = "Open Port";
            this.btnOpenPort.UseVisualStyleBackColor = true;
            this.btnOpenPort.Click += new System.EventHandler(this.btnOpenPort_Click);
            // 
            // comPortCmbBox
            // 
            this.comPortCmbBox.FormattingEnabled = true;
            this.comPortCmbBox.Location = new System.Drawing.Point(270, 170);
            this.comPortCmbBox.Name = "comPortCmbBox";
            this.comPortCmbBox.Size = new System.Drawing.Size(121, 21);
            this.comPortCmbBox.TabIndex = 6;
            // 
            // retrieveLogPercentLbl
            // 
            this.retrieveLogPercentLbl.AutoSize = true;
            this.retrieveLogPercentLbl.Location = new System.Drawing.Point(12, 93);
            this.retrieveLogPercentLbl.Name = "retrieveLogPercentLbl";
            this.retrieveLogPercentLbl.Size = new System.Drawing.Size(35, 13);
            this.retrieveLogPercentLbl.TabIndex = 7;
            this.retrieveLogPercentLbl.Text = "label1";
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(500, 262);
            this.Controls.Add(this.retrieveLogPercentLbl);
            this.Controls.Add(this.comPortCmbBox);
            this.Controls.Add(this.btnOpenPort);
            this.Controls.Add(this.retrieveLogStatusLbl);
            this.Controls.Add(this.retrieveLogPrgBar);
            this.Controls.Add(this.button4);
            this.Controls.Add(this.button3);
            this.Controls.Add(this.btnDownload);
            this.Controls.Add(this.btnOpen);
            this.Controls.Add(this.firmwarePathTxtBox);
            this.Name = "frmMain";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Firmware Update";
            this.Load += new System.EventHandler(this.frmMain_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox firmwarePathTxtBox;
        private System.Windows.Forms.Button btnOpen;
        private System.Windows.Forms.Button btnDownload;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Button button4;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.ProgressBar retrieveLogPrgBar;
        private System.Windows.Forms.Label retrieveLogStatusLbl;
        private System.Windows.Forms.Button btnOpenPort;
        private System.Windows.Forms.ComboBox comPortCmbBox;
        private System.Windows.Forms.Label retrieveLogPercentLbl;
    }
}

