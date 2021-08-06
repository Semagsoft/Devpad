Public Class NewFileDialog

    Public Res As String = "Cancel"
    Public FileType As String = Nothing

    Private Sub Window_Loaded(ByVal sender As System.Object, ByVal e As System.Windows.RoutedEventArgs) Handles MyBase.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If
        Dim tvi As TreeViewItem
        If My.Settings.Options_DefaultFormat = 1 Then
            tvi = TryCast(TreeView1.Items.Item(0), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 2 Then
            tvi = TryCast(TreeView1.Items.Item(1), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 3 Then
            tvi = TryCast(TreeView1.Items.Item(2), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 4 Then
            tvi = TryCast(TreeView1.Items.Item(3), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 5 Then
            tvi = TryCast(TreeView1.Items.Item(4), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 6 Then
            tvi = TryCast(TreeView1.Items.Item(5), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 7 Then
            tvi = TryCast(TreeView1.Items.Item(6), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 8 Then
            tvi = TryCast(TreeView1.Items.Item(7), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 9 Then
            tvi = TryCast(TreeView1.Items.Item(8), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 10 Then
            tvi = TryCast(TreeView1.Items.Item(9), TreeViewItem)
            tvi.IsSelected = True
        ElseIf My.Settings.Options_DefaultFormat = 11 Then
            tvi = TryCast(TreeView1.Items.Item(10), TreeViewItem)
            tvi.IsSelected = True
        End If
    End Sub

    Private Sub TreeView1_SelectedItemChanged(ByVal sender As Object, ByVal e As System.Windows.RoutedPropertyChangedEventArgs(Of Object)) Handles TreeView1.SelectedItemChanged
        ListBox1.Items.Clear()
        If CSItem.IsSelected Then
            For Each s As String In My.Computer.FileSystem.GetFiles(My.Application.Info.DirectoryPath + "\Templates\CSharp")
                Dim f As New IO.FileInfo(s)
                ListBox1.Items.Add(f.Extension)
            Next
        ElseIf CPPItem.IsSelected Then
            For Each s As String In My.Computer.FileSystem.GetFiles(My.Application.Info.DirectoryPath + "\Templates\CPlusPlus")
                Dim f As New IO.FileInfo(s)
                ListBox1.Items.Add(f.Extension)
            Next
        Else
            Dim selected As TreeViewItem = TryCast(e.NewValue, TreeViewItem)
            For Each s As String In My.Computer.FileSystem.GetFiles(My.Application.Info.DirectoryPath + "\Templates\" + selected.Header.ToString)
                Dim f As New IO.FileInfo(s)
                ListBox1.Items.Add(f.Extension)
            Next
        End If
        If ListBox1.Items.Count > 0 Then
            ListBox1.SelectedIndex = 0
        End If
    End Sub

    Private Sub ListBox1_SelectionChanged(ByVal sender As Object, ByVal e As System.Windows.Controls.SelectionChangedEventArgs) Handles ListBox1.SelectionChanged
        If ListBox1.SelectedItem IsNot Nothing Then
            TextBox1.Text = "newfile" + ListBox1.SelectedItem.ToString
            FileType = ListBox1.SelectedItem.ToString
        End If
    End Sub

    Private Sub TextBox1_TextChanged(ByVal sender As Object, ByVal e As System.Windows.Controls.TextChangedEventArgs) Handles TextBox1.TextChanged
        If ListBox1.SelectedItem IsNot Nothing AndAlso TextBox1.Text.Length > 0 Then
            OKButton.IsEnabled = True
        Else
            OKButton.IsEnabled = False
        End If
    End Sub

    Private Sub OKButton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles OKButton.Click
        Res = "OK"
        Close()
    End Sub
End Class