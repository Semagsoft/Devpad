Public Class OptionsDialog
    Public Res As String = "Cancel"

    Private Sub OptionsDialog_Loaded(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If

        StartupComboBox.SelectedIndex = My.Settings.Options_StartupMode
        ThemeComboBox.SelectedIndex = My.Settings.Options_Theme
        DefaultFormatComboBox.SelectedIndex = My.Settings.Options_DefaultFormat
        CloseButtonComboBox.SelectedIndex = My.Settings.Options_CloseButtonMade
        DefaultEncodingComboBox.SelectedIndex = My.Settings.Options_DefaultEncoding
        FontFaceComboBox.SelectedItem = My.Settings.Options_DefaultFont
        FontSizeBox.Text = Convert.ToString(My.Settings.Options_DefaultFontSize)
        ShowLineNumbersCheckBox.IsChecked = My.Settings.Options_ShowLineNumbers
        ScrollPastContentCheckBox.IsChecked = My.Settings.Options_ScrollPastContent
        WordWrapCheckBox.IsChecked = My.Settings.Options_WordWrap
        CodeCollapsingCheckBox.IsChecked = My.Settings.Options_CodeCollapsing
    End Sub

    Private Sub OKButton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles OKButton.Click
        My.Settings.Options_StartupMode = StartupComboBox.SelectedIndex
        My.Settings.Options_Theme = ThemeComboBox.SelectedIndex
        My.Settings.Options_DefaultFormat = DefaultFormatComboBox.SelectedIndex
        My.Settings.Options_CloseButtonMade = CloseButtonComboBox.SelectedIndex
        My.Settings.Options_DefaultEncoding = DefaultEncodingComboBox.SelectedIndex
        My.Settings.Options_DefaultFont = TryCast(FontFaceComboBox.SelectedItem, FontFamily)
        My.Settings.Options_DefaultFontSize = Convert.ToDouble(FontSizeBox.Text)
        If ShowLineNumbersCheckBox.IsChecked Then
            My.Settings.Options_ShowLineNumbers = True
        Else
            My.Settings.Options_ShowLineNumbers = False
        End If
        If ScrollPastContentCheckBox.IsChecked Then
            My.Settings.Options_ScrollPastContent = True
        Else
            My.Settings.Options_ScrollPastContent = False
        End If
        If WordWrapCheckBox.IsChecked Then
            My.Settings.Options_WordWrap = True
        Else
            My.Settings.Options_WordWrap = False
        End If
        If CodeCollapsingCheckBox.IsChecked Then
            My.Settings.Options_CodeCollapsing = True
        Else
            My.Settings.Options_CodeCollapsing = False
        End If
        Res = "OK"
        Close()
    End Sub

    Private Sub ClearRecentFilesButton_Click(ByVal sender As System.Object, ByVal e As System.Windows.RoutedEventArgs) Handles ClearRecentFilesButton.Click
        If MessageBox.Show("Are you sure?", "Clear Recent Files", MessageBoxButton.YesNo, MessageBoxImage.Question) = MessageBoxResult.Yes Then
            My.Settings.Options_RecentFiles.Clear()
        End If
    End Sub

    Private Sub FontSizeBox_TextChanged(sender As Object, e As System.Windows.Controls.TextChangedEventArgs) Handles FontSizeBox.TextChanged
        Try
            Convert.ToDouble(FontSizeBox.Text)
        Catch ex As Exception
            FontSizeBox.Text = My.Settings.Options_DefaultFontSize.ToString
        End Try
    End Sub
End Class