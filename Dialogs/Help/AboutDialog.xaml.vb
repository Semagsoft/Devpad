Public Class AboutDialog

    Private Sub AboutDialog_Loaded(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If
        TextBox1.Text = My.Resources.License_en
        VersionLabel.Text = "Version: " + My.Application.Info.Version.Major.ToString + "." + My.Application.Info.Version.Minor.ToString
        CopyrightLabel.Content = My.Application.Info.Copyright
    End Sub

    Private Sub WebsiteButton_Click(ByVal sender As System.Object, ByVal e As System.Windows.RoutedEventArgs) Handles WebsiteButton.Click
        Process.Start("http://semagsoft.com")
    End Sub

    Private Sub LicenseButton_Click(ByVal sender As System.Object, ByVal e As System.Windows.RoutedEventArgs) Handles LicenseButton.Click
        If TextBox1.IsVisible Then
            TextBox1.Visibility = Windows.Visibility.Hidden
            LicenseImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Help/license16.png"))
            LicenseTextBlock.Text = "License"
        Else
            TextBox1.Visibility = Windows.Visibility.Visible
            LicenseImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Navigation/lineleft16.png"))
            LicenseTextBlock.Text = "Back"
        End If
    End Sub

    Private Sub OnlineHelpButton_Click(sender As System.Object, e As System.Windows.RoutedEventArgs) Handles OnlineHelpButton.Click
        Process.Start("http://devpad.codeplex.com/documentation")
    End Sub

    Private Sub CheckForUpdatesButton_Click(sender As Object, e As RoutedEventArgs) Handles CheckForUpdatesButton.Click
        Dim update As New UpdateDialog
        update.Owner = Me
        update.ShowDialog()
    End Sub
End Class