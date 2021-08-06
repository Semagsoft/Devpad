Public Class WordpressDialog
    Public Res As String = "Cancel"

    Private Sub OKButton_Click(ByVal sender As System.Object, ByVal e As System.Windows.RoutedEventArgs) Handles OKButton.Click
        Res = "OK"
        Close()
    End Sub

    Private Sub BlogTextBox_KeyDown(ByVal sender As Object, ByVal e As System.Windows.Input.KeyEventArgs) Handles BlogTextBox.KeyDown, TitleTextBox.KeyDown, UserNameTextBox.KeyDown, PasswordTextBox.KeyDown
        If e.Key = Key.Enter Then
            If OKButton.IsEnabled Then
                OKButton_Click(Nothing, Nothing)
            End If
        End If
    End Sub

    Private Sub TextBox_TextChanged(ByVal sender As Object, ByVal e As System.Windows.Controls.TextChangedEventArgs) Handles BlogTextBox.TextChanged, TitleTextBox.TextChanged, UserNameTextBox.TextChanged
        If BlogTextBox.Text.Length > 0 AndAlso TitleTextBox.Text.Length > 0 AndAlso UserNameTextBox.Text.Length > 0 Then
            If PasswordTextBox.Password.Length > 0 Then
                OKButton.IsEnabled = True
            Else
                OKButton.IsEnabled = False
            End If
        Else
            OKButton.IsEnabled = False
        End If
    End Sub

    Private Sub PasswordTextBox_PasswordChanged(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles PasswordTextBox.PasswordChanged
        TextBox_TextChanged(Nothing, Nothing)
    End Sub

    Private Sub WordpressDialog_Loaded(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If
        BlogTextBox.Focus()
    End Sub
End Class