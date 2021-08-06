Public Class TabHeader

    Public Shared SaveEvent As RoutedEvent = EventManager.RegisterRoutedEvent("Save", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(TabHeader))
    Public Shared SaveAsEvent As RoutedEvent = EventManager.RegisterRoutedEvent("SaveAs", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(TabHeader))
    Public Shared SaveAllEvent As RoutedEvent = EventManager.RegisterRoutedEvent("SaveAll", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(TabHeader))
    Public Shared CloseTabEvent As RoutedEvent = EventManager.RegisterRoutedEvent("CloseTab", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(TabHeader))
    Public Shared CloseAllButThisEvent As RoutedEvent = EventManager.RegisterRoutedEvent("CloseAllButThis", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(TabHeader))
    Public Shared CloseAllEvent As RoutedEvent = EventManager.RegisterRoutedEvent("CloseAll", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(TabHeader))

    Private Sub SaveMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles SaveMenuItem.Click
        Me.RaiseEvent(New RoutedEventArgs(SaveEvent))
    End Sub

    Private Sub SaveAsMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles SaveAsMenuItem.Click
        Me.RaiseEvent(New RoutedEventArgs(SaveAsEvent))
    End Sub

    Private Sub SaveAllMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles SaveAllMenuItem.Click
        Me.RaiseEvent(New RoutedEventArgs(SaveAllEvent))
    End Sub

    Private Sub CloseMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CloseMenuItem.Click
        Me.RaiseEvent(New RoutedEventArgs(CloseTabEvent))
    End Sub

    Private Sub CloseButton_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CloseButton.Click
        Me.RaiseEvent(New RoutedEventArgs(CloseTabEvent))
    End Sub

    Private Sub CloseAllButThisMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CloseAllButThisMenuItem.Click
        Me.RaiseEvent(New RoutedEventArgs(CloseAllButThisEvent))
    End Sub
End Class