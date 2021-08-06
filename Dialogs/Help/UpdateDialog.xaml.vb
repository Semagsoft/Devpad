Public Class UpdateDialog

    Private WithEvents CheckForUpdateWorker As New System.ComponentModel.BackgroundWorker, webmanager As New Net.WebClient

#Region "Reuseable Code"

    Public Function PathExists(ByVal path As String, ByVal timeout As Integer) As Boolean
        Dim exists As Boolean = True
        Dim t As New System.Threading.Thread(DirectCast(Function() CheckPathFunction(path), System.Threading.ThreadStart))
        t.Start()
        Dim completed As Boolean = t.Join(timeout)
        If Not completed Then
            exists = False
            t.Abort()
        End If
        Return exists
    End Function

    Public Function CheckPathFunction(ByVal path As String) As Boolean
        Return System.IO.File.Exists(path)
    End Function

#End Region

#Region "Window"

    Private Sub UpdateDialog_Loaded(sender As Object, e As RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If
        If My.Computer.Network.IsAvailable Then
            CheckForUpdateWorker.RunWorkerAsync()
        Else
            Dim m As New MessageBoxDialog("Internet not found, please connect and try again.", "Error", Nothing, Nothing)
            m.Owner = Me
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/error32.png"))
            m.ShowDialog()
            Close()
        End If
    End Sub

#End Region

#Region "Check for Updates"

    Private Sub CheckForUpdateWorker_DoWork(ByVal sender As Object, ByVal e As System.ComponentModel.DoWorkEventArgs) Handles CheckForUpdateWorker.DoWork
        Try
            If PathExists("http://semagsoft.com/software/updates/devpad.update", 5000) Then
                Dim fileReader As New Net.WebClient
                My.Computer.FileSystem.CreateDirectory(My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\")
                Dim filename As String = My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\" + "devpadupdate.update"
                If IO.File.Exists(filename) Then
                    IO.File.Delete(filename)
                End If
                fileReader.DownloadFile(New Uri("http://semagsoft.com/software/updates/devpad.update"), filename)
                e.Result = True
            Else
                e.Result = False
            End If
        Catch ex As Exception
            Dim m As New MessageBoxDialog(ex.Message, ex.ToString, Nothing, Nothing)
            m.Owner = Me
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/error32.png"))
            m.ShowDialog()
            e.Result = False
        End Try
    End Sub

    Private Sub CheckForUpdateWorker_RunWorkerCompleted(ByVal sender As Object, ByVal e As System.ComponentModel.RunWorkerCompletedEventArgs) Handles CheckForUpdateWorker.RunWorkerCompleted
        If e.Result = True Then
            Dim textreader As IO.TextReader = IO.File.OpenText(My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\" + "devpadupdate.update")
            Dim version As String = textreader.ReadLine
            Dim versionmajor As Integer = Convert.ToInt16(version.Substring(0, 1))
            Dim versionnumber As Integer = Convert.ToInt16(version.Substring(2))
            If versionmajor >= My.Application.Info.Version.Major AndAlso versionnumber > My.Application.Info.Version.Minor Then
                Dim whatsnew As New Collection
                Dim line As String
                Do
                    line = textreader.ReadLine
                    If line IsNot Nothing Then
                        whatsnew.Add(line.ToString)
                    End If
                Loop Until line Is Nothing
                textreader.Close()
                Title = "New Update"
                UpdateTextBlock.Text = "Update found(" + version + "), apply it?"
                UpdateProgressBar.Visibility = Windows.Visibility.Collapsed
                ApplyButton.Visibility = Windows.Visibility.Visible
                CancelButton.Visibility = Windows.Visibility.Visible
                WhatsNewTextBox.Clear()
                For Each s As String In whatsnew
                    WhatsNewTextBox.AppendText(s + vbNewLine)
                Next
                WhatsNewTextBlock.Visibility = Windows.Visibility.Visible
                WhatsNewTextBox.Visibility = Windows.Visibility.Visible
                Top = Top - 45
                Height = 218
            Else
                Dim m As New MessageBoxDialog("Devpad is already up to date", "Up To Date", Nothing, Nothing)
                m.Owner = Me
                m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/info32.png"))
                m.ShowDialog()
                Close()
            End If
        End If
    End Sub

#End Region

#Region "Update"

    Public Sub webmanager_DownloadProgressChanged(sender As Object, e As Net.DownloadProgressChangedEventArgs) Handles webmanager.DownloadProgressChanged
        UpdateProgressbar.Value = e.ProgressPercentage
    End Sub

    Public Sub webmanager_DownloadFileCompleted(ByVal sender As Object, ByVal e As System.ComponentModel.AsyncCompletedEventArgs) Handles webmanager.DownloadFileCompleted
        If Not e.Cancelled Then
            Try
                Process.Start(My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\" + "setup.exe", "/D=" + My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\")
                My.Application.Shutdown()
            Catch ex As Exception
                If ex.Message.EndsWith("canceled by the user") Then
                    Dim m As New MessageBoxDialog("The update has been canceled!", "Update Canceled", Nothing, Nothing)
                    m.Owner = Me
                    m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/info32.png"))
                    m.ShowDialog()
                    CancelButton_Click(Nothing, Nothing)
                Else
                    Dim m As New MessageBoxDialog("Error running update installer" + ex.Message, "Error", Nothing, Nothing)
                    m.Owner = Me
                    m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/error32.png"))
                    m.ShowDialog()
                    CancelButton_Click(Nothing, Nothing)
                End If
            End Try
        End If
    End Sub

#End Region

    Private Sub ApplyButton_Click(sender As Object, e As RoutedEventArgs) Handles ApplyButton.Click
        UpdateProgressBar.IsIndeterminate = False
        UpdateProgressBar.Minimum = 0
        UpdateProgressBar.Maximum = 100
        UpdateProgressBar.Value = 0
        UpdateProgressBar.Visibility = Windows.Visibility.Visible
        If IO.File.Exists(My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\" + "Setup.exe") Then
            IO.File.Delete(My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\" + "Setup.exe")
        End If
        webmanager.DownloadFileAsync(New Uri("http://semagsoft.com/software/downloads/Devpad_Setup.exe"), My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\" + "Setup.exe")
        UpdateTextBlock.Text = "Downloading, Please Wait..."
        ApplyButton.Visibility = Windows.Visibility.Collapsed
    End Sub

    Private Sub CancelButton_Click(sender As Object, e As RoutedEventArgs) Handles CancelButton.Click
        While webmanager.IsBusy
            webmanager.CancelAsync()
        End While
        If IO.File.Exists(My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\" + "Setup.exe") Then
            IO.File.Delete(My.Computer.FileSystem.SpecialDirectories.Temp + "\Semagsoft\Devpad\" + "Setup.exe")
        End If
        Close()
    End Sub
End Class