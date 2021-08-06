Public Class EmailDialog
    Public Res As String = "Cancel"

    Private Sub OKButton_Click(ByVal sender As System.Object, ByVal e As System.Windows.RoutedEventArgs) Handles OKButton.Click
        Res = "OK"
        Close()
    End Sub

    Private Sub Box_KeyDown(ByVal sender As Object, ByVal e As System.Windows.Input.KeyEventArgs) Handles PortTextBox.KeyDown, FromBox.KeyDown, ToBox.KeyDown, SubjectBox.KeyDown, BodyBox.KeyDown
        If e.Key = Key.Enter Then
            If OKButton.IsEnabled Then
                OKButton_Click(Nothing, Nothing)
            End If
        End If
    End Sub

    Private Sub Box_TextChanged(ByVal sender As Object, ByVal e As System.Windows.Controls.TextChangedEventArgs) Handles PortTextBox.TextChanged, FromBox.TextChanged, ToBox.TextChanged, SubjectBox.TextChanged, BodyBox.TextChanged
        If PortTextBox.Text.Length > 0 AndAlso FromBox.Text.Length > 0 AndAlso ToBox.Text.Length > 0 AndAlso SubjectBox.Text.Length > 0 AndAlso BodyBox.Text.Length > 0 Then
            If EmailPassword.Password.Length > 0 AndAlso SMTPComboBox.Text.Length > 0 Then
                OKButton.IsEnabled = True
            Else
                OKButton.IsEnabled = False
            End If
        Else
            OKButton.IsEnabled = False
        End If

        'If SMTPComboBox.SelectedItem Is Nothing Then
        '    OKButton.IsEnabled = False
        'End If

        Try
            Convert.ToInt32(PortTextBox.Text)
        Catch ex As Exception
            PortTextBox.Clear()
        End Try
    End Sub

    Private Sub EmailDialog_Loaded(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If
    End Sub

    Private Sub EmailPassword_KeyDown(ByVal sender As Object, ByVal e As System.Windows.Input.KeyEventArgs) Handles EmailPassword.KeyDown
        If e.Key = Key.Enter Then
            If OKButton.IsEnabled Then
                OKButton_Click(Nothing, Nothing)
            End If
        End If
    End Sub

    Private Sub SMTPComboBox_SelectionChanged(ByVal sender As Object, ByVal e As System.Windows.Controls.SelectionChangedEventArgs) Handles SMTPComboBox.SelectionChanged
        'Box_TextChanged(Nothing, Nothing)
        If SMTPComboBox.SelectedItem Is Nothing Then
            OKButton.IsEnabled = False
        End If
    End Sub

    Private Sub EmailPassword_PasswordChanged(sender As Object, e As RoutedEventArgs) Handles EmailPassword.PasswordChanged
        If PortTextBox.Text.Length > 0 AndAlso FromBox.Text.Length > 0 AndAlso ToBox.Text.Length > 0 AndAlso SubjectBox.Text.Length > 0 AndAlso BodyBox.Text.Length > 0 Then
            If EmailPassword.Password.Length > 0 AndAlso SMTPComboBox.Text.Length > 0 Then
                OKButton.IsEnabled = True
            Else
                OKButton.IsEnabled = False
            End If
        Else
            OKButton.IsEnabled = False
        End If

        'If SMTPComboBox.SelectedItem Is Nothing Then
        '    OKButton.IsEnabled = False
        'End If
    End Sub
End Class