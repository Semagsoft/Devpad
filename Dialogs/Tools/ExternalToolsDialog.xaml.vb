Public Class ExternalToolsDialog
    Public Res As String = "Cancel"

    Private Sub ExternalToolsDialog_Loaded(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If
        For Each s As String In My.Settings.ExTools
            ListBox1.Items.Add(s)
        Next
    End Sub

    Private Sub OKButton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles OKButton.Click
        Res = "OK"
        Close()
    End Sub

    Private Sub AddButton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles AddButton.Click
        Dim extool As New Microsoft.Win32.OpenFileDialog
        extool.Filter = "Programs(*.exe)|*.exe|All Files(*.*)|*.*"
        If extool.ShowDialog Then
            ListBox1.Items.Add(extool.FileName)
        End If
    End Sub

    Private Sub RemoveButton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RemoveButton.Click
        ListBox1.Items.Remove(ListBox1.SelectedItem)
    End Sub

    Private Sub Editbutton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles Editbutton.Click
        Dim extool As New Microsoft.Win32.OpenFileDialog
        extool.Filter = "Programs(*.exe)|*.exe|All Files(*.*)|*.*"
        Dim item As String = TryCast(ListBox1.SelectedItem, String)
        If extool.ShowDialog Then
            'TODO: Save edited ExTool
            ListBox1.SelectedItem = item
            ListBox1.SelectedValue = extool.FileName
            ListBox1.Items.Refresh()
        End If
    End Sub

    Private Sub ListBox1_SelectionChanged(ByVal sender As Object, ByVal e As System.Windows.Controls.SelectionChangedEventArgs) Handles ListBox1.SelectionChanged
        If ListBox1.SelectedItem IsNot Nothing Then
            RemoveButton.IsEnabled = True
            Editbutton.IsEnabled = True
            MoveUpButton.IsEnabled = True
            MoveDownButton.IsEnabled = True
        Else
            RemoveButton.IsEnabled = False
            Editbutton.IsEnabled = False
            MoveUpButton.IsEnabled = False
            MoveDownButton.IsEnabled = False
        End If
    End Sub

    'Private Sub MoveUpButton_Click(ByVal sender As System.Object, ByVal e As System.Windows.RoutedEventArgs) Handles MoveUpButton.Click
    '    ListBox1.Items.MoveCurrentToNext()
    'End Sub

    'Private Sub MoveDownButton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles MoveDownButton.Click
    '    ListBox1.Items.MoveCurrentToNext()
    'End Sub
End Class