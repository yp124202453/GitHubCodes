namespace WinFormApp_ResetEvent
{
    partial class Form1
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要修改
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            this.btnStartTest = new System.Windows.Forms.Button();
            this.btnSet = new System.Windows.Forms.Button();
            this.txt = new System.Windows.Forms.TextBox();
            this.chkCloseTask = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // btnStartTest
            // 
            this.btnStartTest.Location = new System.Drawing.Point(132, 236);
            this.btnStartTest.Name = "btnStartTest";
            this.btnStartTest.Size = new System.Drawing.Size(85, 23);
            this.btnStartTest.TabIndex = 0;
            this.btnStartTest.Text = "Start Test Task";
            this.btnStartTest.UseVisualStyleBackColor = true;
            this.btnStartTest.Click += new System.EventHandler(this.btnStartTest_Click);
            // 
            // btnSet
            // 
            this.btnSet.Location = new System.Drawing.Point(236, 236);
            this.btnSet.Name = "btnSet";
            this.btnSet.Size = new System.Drawing.Size(85, 23);
            this.btnSet.TabIndex = 1;
            this.btnSet.Text = "Set";
            this.btnSet.UseVisualStyleBackColor = true;
            this.btnSet.Click += new System.EventHandler(this.btnSet_Click);
            // 
            // txt
            // 
            this.txt.Location = new System.Drawing.Point(12, 12);
            this.txt.Multiline = true;
            this.txt.Name = "txt";
            this.txt.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txt.Size = new System.Drawing.Size(309, 195);
            this.txt.TabIndex = 2;
            // 
            // chkCloseTask
            // 
            this.chkCloseTask.AutoSize = true;
            this.chkCloseTask.Location = new System.Drawing.Point(28, 240);
            this.chkCloseTask.Name = "chkCloseTask";
            this.chkCloseTask.Size = new System.Drawing.Size(84, 16);
            this.chkCloseTask.TabIndex = 3;
            this.chkCloseTask.Text = "Close Task";
            this.chkCloseTask.UseVisualStyleBackColor = true;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(335, 270);
            this.Controls.Add(this.chkCloseTask);
            this.Controls.Add(this.txt);
            this.Controls.Add(this.btnSet);
            this.Controls.Add(this.btnStartTest);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "ResetEvent";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnStartTest;
        private System.Windows.Forms.Button btnSet;
        private System.Windows.Forms.TextBox txt;
        private System.Windows.Forms.CheckBox chkCloseTask;
    }
}

