Imports System.Windows.Threading
Imports AdsJumboWinForm
Public Class AdWindow
    Private Sub AdWindow_Loaded(sender As Object, e As RoutedEventArgs) Handles Me.Loaded
        Dim dt As DispatcherTimer = New DispatcherTimer()

        AddHandler dt.Tick, AddressOf dispatcherTimer_Tick

        dt.Interval = New TimeSpan(0, 0, 1)

        dt.Start()

        Dim mainAd As New InterstitialAd
        mainAd.Controls.Remove(mainAd.Controls.Item(0))
        AdHost.Child = mainAd
        mainAd.ShowInterstitialAd("5i116jyf8hlp")
    End Sub


    Private time As Integer = 0
    Public Sub dispatcherTimer_Tick(ByVal sender As Object, ByVal e As EventArgs)
        time += 1
        If time >= 5 Then
            WindowStyle = WindowStyle.SingleBorderWindow
        End If
    End Sub
End Class