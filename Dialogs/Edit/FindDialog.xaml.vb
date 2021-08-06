Public Class FindDialog
    Public Res As String = "Cancel"

    Private Sub OKButton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles OKButton.Click
        Res = "OK"
        Close()
    End Sub

    Private Sub FindDialog_Loaded(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If

        TextBox1.Focus()
    End Sub

    Private Sub TextBox1_KeyDown(ByVal sender As Object, ByVal e As System.Windows.Input.KeyEventArgs) Handles TextBox1.KeyDown
        If e.Key = Key.Enter AndAlso OKButton.IsEnabled Then
            OKButton_Click(Nothing, Nothing)
        End If
    End Sub

    Private Sub TextBox1_TextChanged(ByVal sender As Object, ByVal e As System.Windows.Controls.TextChangedEventArgs) Handles TextBox1.TextChanged
        If TextBox1.Text.Length > 0 Then
            OKButton.IsEnabled = True
        Else
            OKButton.IsEnabled = False
        End If
    End Sub
End Class