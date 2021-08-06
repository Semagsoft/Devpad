Public Class MessageBoxDialog
    Public Result As String = "Cancel"

    Public Sub New(text As String, title As String, buttons As String, icon As Integer)
        ' This call is required by the designer.
        InitializeComponent()
        ' Add any initialization after the InitializeComponent() call.
        MessageBoxText.Text = text
        Me.Title = title
        If buttons = "YesNo" Then
            OKButton.Visibility = Windows.Visibility.Collapsed
            YesButton.Visibility = Windows.Visibility.Visible
            NoButton.Visibility = Windows.Visibility.Visible
            YesButton.IsDefault = True
            YesButton.Focus()
        ElseIf buttons = "YesNoCancel" Then
            OKButton.Visibility = Windows.Visibility.Collapsed
            YesButton.Visibility = Windows.Visibility.Visible
            NoButton.Visibility = Windows.Visibility.Visible
            CancelButton.Visibility = Windows.Visibility.Visible
            YesButton.IsDefault = True
            YesButton.Focus()
        End If
    End Sub

    Private Sub OKButton_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles OKButton.Click
        DialogResult = True
        Close()
    End Sub

    Private Sub YesButton_Click(sender As Object, e As RoutedEventArgs) Handles YesButton.Click
        Result = "Yes"
        DialogResult = True
        Close()
    End Sub

    Private Sub NoButton_Click(sender As Object, e As RoutedEventArgs) Handles NoButton.Click
        Result = "No"
        DialogResult = True
        Close()
    End Sub

    Private Sub CancelButton_Click(sender As Object, e As RoutedEventArgs) Handles CancelButton.Click
        Result = "Cancel"
        DialogResult = True
        Close()
    End Sub
End Class