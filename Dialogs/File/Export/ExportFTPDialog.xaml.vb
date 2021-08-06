Public Class ExportFTPDialog
    Public Res As Boolean = False, FileName As String = Nothing

    Private WithEvents UploadWorker As New System.ComponentModel.BackgroundWorker
    Private Sub UploadWorker_DoWork(ByVal sender As Object, ByVal e As System.ComponentModel.DoWorkEventArgs) Handles UploadWorker.DoWork
        Try
            Dim info As Collection = TryCast(e.Argument, Collection)
            Dim ftp As New Utilities.FTP.FTPclient(TryCast(info.Item(1), String), TryCast(info.Item(2), String), TryCast(info.Item(3), String)), fileinfo As New IO.FileInfo(FileName)
            ftp.Upload(FileName, "/" + fileinfo.Name)
            MessageBox.Show("File Uploaded", "Uploaded", MessageBoxButton.OK, MessageBoxImage.Information)
            e.Result = True
        Catch ex As Exception
            MessageBox.Show(ex.Message, ex.ToString, MessageBoxButton.OK, MessageBoxImage.Error)
            e.Result = False
        End Try
    End Sub

    Private Sub UploadWorker_RunWorkerCompleted(ByVal sender As Object, ByVal e As System.ComponentModel.RunWorkerCompletedEventArgs) Handles UploadWorker.RunWorkerCompleted
        Close()
    End Sub

    Private Sub Dialog_Loaded(sender As Object, e As System.Windows.RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If
        HostTextBox.Focus()
    End Sub

    Private Sub OKButton_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles OKButton.Click
        Try
            If My.Computer.Network.IsAvailable Then
                UploadingText.Visibility = Windows.Visibility.Visible
                Loadingbar.Visibility = Windows.Visibility.Visible
                Dim pram As New Collection
                pram.Add(HostTextBox.Text)
                pram.Add(UsernameTextBox.Text)
                pram.Add(FTPPasswordBox.Password)
                UploadWorker.RunWorkerAsync(pram)
            Else
                MessageBox.Show("Internet not found!", "Error", MessageBoxButton.OK, MessageBoxImage.Error)
            End If
        Catch ex As Exception
            MessageBox.Show(ex.Message, ex.ToString, MessageBoxButton.OK, MessageBoxImage.Error)
        End Try
    End Sub

    Private Sub TextBox_KeyDown(ByVal sender As Object, ByVal e As System.Windows.Input.KeyEventArgs) Handles HostTextBox.KeyDown, UsernameTextBox.KeyDown, FTPPasswordBox.KeyDown
        If e.Key = Key.Enter Then
            If OKButton.IsEnabled Then
                OKButton_Click(Nothing, Nothing)
            End If
        End If
    End Sub

    Private Sub TextBox_TextChanged(ByVal sender As Object, ByVal e As System.Windows.Controls.TextChangedEventArgs) Handles HostTextBox.TextChanged, UsernameTextBox.TextChanged
        If HostTextBox.Text.Length > 0 AndAlso UsernameTextBox.Text.Length > 0 AndAlso FTPPasswordBox.Password.Length > 0 Then
            OKButton.IsEnabled = True
        Else
            OKButton.IsEnabled = False
        End If
    End Sub

    Private Sub FTPPasswordBox_PasswordChanged(sender As Object, e As System.Windows.RoutedEventArgs) Handles FTPPasswordBox.PasswordChanged
        If HostTextBox.Text.Length > 0 AndAlso UsernameTextBox.Text.Length > 0 AndAlso FTPPasswordBox.Password.Length > 0 Then
            OKButton.IsEnabled = True
        Else
            OKButton.IsEnabled = False
        End If
    End Sub
End Class