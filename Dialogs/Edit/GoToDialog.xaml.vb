Public Class GoToDialog
    Public Res As String = Nothing
    Public Number As Integer = Nothing

    Private Sub OKButton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles OKButton.Click
        Res = "OK"
        Number = Convert.ToInt32(TextBox1.Text)
        Close()
    End Sub

    Private Sub GoToDialog_Loaded(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles Me.Loaded
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
        Try
            Convert.ToInt32(TextBox1.Text)
        Catch ex As Exception
            TextBox1.Text = Nothing
        End Try
        If TextBox1.Text = "" Then
            OKButton.IsEnabled = False
        Else
            OKButton.IsEnabled = True
        End If
    End Sub
End Class