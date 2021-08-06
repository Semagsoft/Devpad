Public Class ImportFTPDialog

#Region "Window"

    Public Res As Boolean = False
    Public FileToLoad As String = Nothing

    Private Sub ImportFTPDialog_Loaded(sender As Object, e As RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If
    End Sub

#End Region

#Region "Connect Info"

    Private IsConnected As Boolean = False
    Private WorkingDir As String = "/"

    Private Sub HostTextBox_TextChanged(sender As Object, e As TextChangedEventArgs) Handles HostTextBox.TextChanged, UsernameTextBox.TextChanged
        If HostTextBox.Text.Length > 0 AndAlso UsernameTextBox.Text.Length > 0 Then
            If PasswordTextBox.Password.Length > 0 Then
                ConnectButton.IsEnabled = True
            End If
        End If
    End Sub

    Private Sub PasswordTextBox_PasswordChanged(sender As Object, e As RoutedEventArgs) Handles PasswordTextBox.PasswordChanged
        If HostTextBox.Text.Length > 0 AndAlso UsernameTextBox.Text.Length > 0 Then
            If PasswordTextBox.Password.Length > 0 Then
                ConnectButton.IsEnabled = True
            End If
        End If
    End Sub

    Private Sub ConnectButton_Click(sender As Object, e As RoutedEventArgs) Handles ConnectButton.Click
        If IsConnected Then
            IsConnected = False
            HostTextBox.IsEnabled = True
            UsernameTextBox.IsEnabled = True
            PasswordTextBox.IsEnabled = True
            ConnectButton.Content = "Connect"
            FilesListBox.Items.Clear()
        Else
            Try
                Dim myFtp As New Utilities.FTP.FTPclient(HostTextBox.Text, UsernameTextBox.Text, PasswordTextBox.Password)
                IsConnected = True
                HostTextBox.IsEnabled = False
                UsernameTextBox.IsEnabled = False
                PasswordTextBox.IsEnabled = False
                ConnectButton.Content = "Disconnect"
                For Each i As Utilities.FTP.FTPfileInfo In myFtp.ListDirectoryDetail(WorkingDir)
                    If Not i.NameOnly = "." Then
                        Dim item As New FTPItem
                        item.Content = i.Filename
                        item.ShortName = i.NameOnly
                        item.FileName = i.Filename
                        item.FullName = i.FullName
                        FilesListBox.Items.Add(item)
                    End If
                Next
            Catch ex As Exception
                MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error)
            End Try
        End If
    End Sub

#End Region

#Region "FilesListBox"

    Private Sub FilesListBox_SelectionChanged(sender As Object, e As SelectionChangedEventArgs) Handles FilesListBox.SelectionChanged
        If FilesListBox.SelectedItem IsNot Nothing Then
            Dim i As FTPItem = FilesListBox.SelectedItem
            If i.IsFile Then
                ImportButton.IsEnabled = True
            Else
                ImportButton.IsEnabled = False
            End If
        End If
    End Sub

    Private Sub FilesListBox_MouseDoubleClick(sender As Object, e As MouseButtonEventArgs) Handles FilesListBox.MouseDoubleClick
        If FilesListBox.SelectedItem IsNot Nothing Then
            Dim i As FTPItem = FilesListBox.SelectedItem
            If i.IsFile Then
                ImportButton_Click(Nothing, Nothing)
            Else
                Dim myFtp As New Utilities.FTP.FTPclient(HostTextBox.Text, UsernameTextBox.Text, PasswordTextBox.Password)
                If i.IsFile Then
                    WorkingDir += i.FileName
                Else
                    WorkingDir += i.FileName + "/"
                End If
                FilesListBox.Items.Clear()
                For Each i2 As Utilities.FTP.FTPfileInfo In myFtp.ListDirectoryDetail(WorkingDir)
                    Dim item As New FTPItem
                    item.Content = i2.Filename
                    item.ShortName = i2.NameOnly
                    item.FileName = i2.Filename
                    item.FullName = i2.FullName
                    If i2.FileType = Utilities.FTP.FTPfileInfo.DirectoryEntryTypes.File Then
                        item.IsFile = True
                    End If
                    FilesListBox.Items.Add(item)
                Next
            End If
        End If
    End Sub

#End Region

    Private Sub ImportButton_Click(sender As Object, e As RoutedEventArgs) Handles ImportButton.Click
        Try
            Dim i As FTPItem = FilesListBox.SelectedItem
            Dim myFtp As New Utilities.FTP.FTPclient(HostTextBox.Text, UsernameTextBox.Text, PasswordTextBox.Password)
            Dim file As String = My.Computer.FileSystem.SpecialDirectories.Temp + "\" + i.FileName
            If My.Computer.FileSystem.FileExists(file) Then
                My.Computer.FileSystem.DeleteFile(file)
            End If
            myFtp.Download(WorkingDir + i.FileName, file)
            FileToLoad = file
            Res = True
            Close()
        Catch ex As Exception
            MessageBox.Show(ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error)
        End Try
    End Sub
End Class

Class FTPItem
    Inherits ListBoxItem
    Public ShortName As String
    Public FileName As String
    Public FullName As String
    Public IsFile As Boolean = False
End Class