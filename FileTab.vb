Public Class FileTab
    Inherits Xceed.Wpf.AvalonDock.Layout.LayoutDocument

    Public WithEvents Editor As New CodeEditor, EditorHeader As New TabHeader, EditorContextMenu As New ContextMenu,
        UndoMenuItem As New MenuItem, RedoMenuItem As New MenuItem, CutMenuItem As New MenuItem, CopyMenuItem As New MenuItem,
        PasteMenuItem As New MenuItem, DeleteMenuItem As New MenuItem, SelectAllMenuItem As New MenuItem, FindMenuItem As New MenuItem,
        ReplaceMenuItem As New MenuItem, GoToMenuItem As New MenuItem, InvertCaseMenuItem As New MenuItem, TitleCaseMenuItem As New MenuItem,
        UpperCaseMenuItem As New MenuItem, LowerCaseMenuItem As New MenuItem, IndentMoreMenuItem As New MenuItem, IndentLessMenuItem As New MenuItem

    Public Shared FindEvent As RoutedEvent = EventManager.RegisterRoutedEvent("FindEvent", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(FileTab))
    Public Shared ReplaceEvent As RoutedEvent = EventManager.RegisterRoutedEvent("ReplaceEvent", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(FileTab))
    Public Shared GoToEvent As RoutedEvent = EventManager.RegisterRoutedEvent("GoToEvent", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(FileTab))

    Public Shared UpdateSelected As RoutedEvent = EventManager.RegisterRoutedEvent("UpdateSelected", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(FileTab))

    Public owner As Window = Nothing

    Private Sub Editor_TextChanged(ByVal sender As Object, ByVal e As System.EventArgs) Handles Editor.TextChanged
        'TODO: Me.RaiseEvent(New RoutedEventArgs(UpdateSelected))
    End Sub

#Region "New"

    Public Sub New(o As Window)
        owner = o
        CanFloat = False
        Dim undoimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/undo16.png")), undoicon As New Image
        Dim redoimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/redo16.png")), redoicon As New Image
        Dim cutimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/cut16.png")), cuticon As New Image
        Dim copyimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/copy16.png")), copyicon As New Image
        Dim pasteimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/paste16.png")), pasteicon As New Image
        Dim deleteimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/delete16.png")), deleteicon As New Image
        Dim selectallimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/selectall16.png")), selectallicon As New Image
        Dim findimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/find16.png")), findicon As New Image
        Dim replaceimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/replace16.png")), replaceicon As New Image
        Dim gotoimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/goto16.png")), gotoicon As New Image
        Dim setcaseimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/setcase16.png")), setcaseicon As New Image
        Dim invertcaseimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/invertcase16.png")), invertcaseicon As New Image
        Dim titlecaseimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/titlecase16.png")), titlecaseicon As New Image
        Dim uppercaseimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/uppercase16.png")), uppercaseicon As New Image
        Dim lowercaseimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/lowercase16.png")), lowercaseicon As New Image
        Dim indentmoreimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/indentmore16.png")), indentmoreicon As New Image
        Dim indentlessimg As New BitmapImage(New Uri("pack://application:,,,/Images/Edit/indentless16.png")), indentlessicon As New Image
        undoicon.Width = 16
        undoicon.Height = 16
        undoicon.Source = undoimg
        redoicon.Width = 16
        redoicon.Height = 16
        redoicon.Source = redoimg
        cuticon.Width = 16
        cuticon.Height = 16
        cuticon.Source = cutimg
        copyicon.Width = 16
        copyicon.Height = 16
        copyicon.Source = copyimg
        pasteicon.Width = 16
        pasteicon.Height = 16
        pasteicon.Source = pasteimg
        deleteicon.Width = 16
        deleteicon.Height = 16
        deleteicon.Source = deleteimg
        selectallicon.Width = 16
        selectallicon.Height = 16
        selectallicon.Source = selectallimg
        findicon.Width = 16
        findicon.Height = 16
        findicon.Source = findimg
        replaceicon.Width = 16
        replaceicon.Height = 16
        replaceicon.Source = replaceimg
        gotoicon.Width = 16
        gotoicon.Height = 16
        gotoicon.Source = gotoimg
        setcaseicon.Width = 16
        setcaseicon.Height = 16
        setcaseicon.Source = setcaseimg
        invertcaseicon.Width = 16
        invertcaseicon.Height = 16
        invertcaseicon.Source = invertcaseimg
        titlecaseicon.Width = 16
        titlecaseicon.Height = 16
        titlecaseicon.Source = titlecaseimg
        uppercaseicon.Width = 16
        uppercaseicon.Height = 16
        uppercaseicon.Source = uppercaseimg
        lowercaseicon.Width = 16
        lowercaseicon.Height = 16
        lowercaseicon.Source = lowercaseimg
        indentmoreicon.Width = 16
        indentmoreicon.Height = 16
        indentmoreicon.Source = indentmoreimg
        indentlessicon.Width = 16
        indentlessicon.Height = 16
        indentlessicon.Source = indentlessimg
        UndoMenuItem.Header = "Undo"
        UndoMenuItem.Icon = undoicon
        UndoMenuItem.InputGestureText = "Ctrl+Z"
        UndoMenuItem.ToolTip = "Undo (Ctrl + Z)"
        RedoMenuItem.Header = "Redo"
        RedoMenuItem.Icon = redoicon
        RedoMenuItem.InputGestureText = "Ctrl+Y"
        RedoMenuItem.ToolTip = "Redo (Ctrl + Y)"
        CutMenuItem.Header = "Cut"
        CutMenuItem.Icon = cuticon
        CutMenuItem.InputGestureText = "Ctrl+X"
        CutMenuItem.ToolTip = "Cut (Ctrl + X)"
        CopyMenuItem.Header = "Copy"
        CopyMenuItem.Icon = copyicon
        CopyMenuItem.InputGestureText = "Ctrl+C"
        CopyMenuItem.ToolTip = "Copy (Ctrl + C)"
        PasteMenuItem.Header = "Paste"
        PasteMenuItem.Icon = pasteicon
        PasteMenuItem.InputGestureText = "Ctrl+V"
        PasteMenuItem.ToolTip = "Paste (Ctrl + V)"
        DeleteMenuItem.Header = "Delete"
        DeleteMenuItem.Icon = deleteicon
        DeleteMenuItem.InputGestureText = "Del"
        DeleteMenuItem.ToolTip = "Delete (Del)"
        SelectAllMenuItem.Header = "Select All"
        SelectAllMenuItem.Icon = selectallicon
        SelectAllMenuItem.InputGestureText = "Ctrl+A"
        SelectAllMenuItem.ToolTip = "Select All (Ctrl + A)"
        FindMenuItem.Header = "Find"
        FindMenuItem.Icon = findicon
        FindMenuItem.InputGestureText = "Ctrl+F"
        FindMenuItem.ToolTip = "Find (Ctrl + F)"
        ReplaceMenuItem.Header = "Replace"
        ReplaceMenuItem.Icon = replaceicon
        ReplaceMenuItem.InputGestureText = "Ctrl+H"
        ReplaceMenuItem.ToolTip = "Replace (Ctrl + H)"
        GoToMenuItem.Header = "Go To"
        GoToMenuItem.Icon = gotoicon
        GoToMenuItem.InputGestureText = "Ctrl+G"
        GoToMenuItem.ToolTip = "Go To (Ctrl + G)"
        InvertCaseMenuItem.Header = "Invertcase"
        InvertCaseMenuItem.Icon = invertcaseicon
        TitleCaseMenuItem.Header = "Titlecase"
        TitleCaseMenuItem.Icon = titlecaseicon
        UpperCaseMenuItem.Header = "Uppercase"
        UpperCaseMenuItem.Icon = uppercaseicon
        LowerCaseMenuItem.Header = "Lowercase"
        LowerCaseMenuItem.Icon = lowercaseicon
        IndentMoreMenuItem.Header = "Indent More"
        IndentMoreMenuItem.Icon = indentmoreicon
        IndentMoreMenuItem.InputGestureText = "TAB"
        IndentMoreMenuItem.ToolTip = "Indent More (TAB)"
        IndentLessMenuItem.Header = "Indent Less"
        IndentLessMenuItem.Icon = indentlessicon
        IndentLessMenuItem.InputGestureText = "Shift+TAB"
        IndentLessMenuItem.ToolTip = "Indent Less (Shift + TAB)"
        EditorContextMenu.Items.Add(UndoMenuItem)
        EditorContextMenu.Items.Add(RedoMenuItem)
        EditorContextMenu.Items.Add(New Separator)
        EditorContextMenu.Items.Add(CutMenuItem)
        EditorContextMenu.Items.Add(CopyMenuItem)
        EditorContextMenu.Items.Add(PasteMenuItem)
        EditorContextMenu.Items.Add(DeleteMenuItem)
        EditorContextMenu.Items.Add(New Separator)
        EditorContextMenu.Items.Add(SelectAllMenuItem)
        EditorContextMenu.Items.Add(New Separator)
        EditorContextMenu.Items.Add(FindMenuItem)
        EditorContextMenu.Items.Add(ReplaceMenuItem)
        EditorContextMenu.Items.Add(GoToMenuItem)
        EditorContextMenu.Items.Add(New Separator)
        Dim setcasemenuitem As New MenuItem
        setcasemenuitem.Header = "Set Case"
        setcasemenuitem.Icon = setcaseicon
        setcasemenuitem.Items.Add(InvertCaseMenuItem)
        setcasemenuitem.Items.Add(TitleCaseMenuItem)
        setcasemenuitem.Items.Add(UpperCaseMenuItem)
        setcasemenuitem.Items.Add(LowerCaseMenuItem)
        EditorContextMenu.Items.Add(setcasemenuitem)
        EditorContextMenu.Items.Add(New Separator)
        EditorContextMenu.Items.Add(IndentMoreMenuItem)
        EditorContextMenu.Items.Add(IndentLessMenuItem)
        Editor.ContextMenu = EditorContextMenu
        Content = Editor
    End Sub

#End Region

#Region "ContextMenu"

    Private Sub UndoMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles UndoMenuItem.Click
        Editor.Undo()
    End Sub

    Private Sub RedoMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RedoMenuItem.Click
        Editor.Redo()
    End Sub

    Private Sub CutMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CutMenuItem.Click
        Editor.Cut()
    End Sub

    Private Sub CopyMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CopyMenuItem.Click
        Editor.Copy()
    End Sub

    Private Sub PasteMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles PasteMenuItem.Click
        Editor.Paste()
    End Sub

    Private Sub DeleteMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles DeleteMenuItem.Click
        Editor.SelectedText = ""
    End Sub

    Private Sub SelectAllMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles SelectAllMenuItem.Click
        Editor.SelectAll()
    End Sub

    Private Sub FindMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles FindMenuItem.Click
        'Me.RaiseEvent(New RoutedEventArgs(FindEvent))
    End Sub

    Private Sub ReplaceMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles ReplaceMenuItem.Click
        'Me.RaiseEvent(New RoutedEventArgs(ReplaceEvent))
    End Sub

    Private Sub GoToMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles GoToMenuItem.Click
        'Me.RaiseEvent(New RoutedEventArgs(GoToEvent))
    End Sub

    Private Sub InvertCaseMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles InvertCaseMenuItem.Click
        ICSharpCode.AvalonEdit.AvalonEditCommands.InvertCase.Execute(Nothing, Editor.TextArea.TextView)
    End Sub

    Private Sub TitleCaseMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles TitleCaseMenuItem.Click
        ICSharpCode.AvalonEdit.AvalonEditCommands.ConvertToTitleCase.Execute(Nothing, Editor.TextArea.TextView)
    End Sub

    Private Sub UpperCaseMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles UpperCaseMenuItem.Click
        ICSharpCode.AvalonEdit.AvalonEditCommands.ConvertToUppercase.Execute(Nothing, Editor.TextArea.TextView)
    End Sub

    Private Sub LowerCaseMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles LowerCaseMenuItem.Click
        ICSharpCode.AvalonEdit.AvalonEditCommands.ConvertToLowercase.Execute(Nothing, Editor.TextArea.TextView)
    End Sub

    Private Sub IndentMoreMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles IndentMoreMenuItem.Click
        Editor.Document.Insert(Editor.CaretOffset, vbTab)
    End Sub

    Private Sub IndentLessMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles IndentLessMenuItem.Click
        Try
            If Editor.Document.GetText(Editor.CaretOffset - 1, 1) = vbTab Then
                Editor.Document.Remove(Editor.CaretOffset - 1, 1)
            End If
        Catch ex As Exception
        End Try
    End Sub

#End Region

    Private Sub FileTab_Closing(sender As Object, e As ComponentModel.CancelEventArgs) Handles Me.Closing
        If Editor.FileChanged Then
            Dim m As New MessageBoxDialog("Do you want to save " + Editor.FileName + "?", "Save", "YesNoCancel", Nothing)
            m.Owner = owner
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
            m.ShowDialog()
            If m.Result = "Yes" Then
                If Editor.FileFullName IsNot Nothing Then
                    Editor.Save(Editor.FileFullName)
                Else
                    Dim Savedialog As New Microsoft.Win32.SaveFileDialog
                    Savedialog.AddExtension = True
                    Savedialog.Filter = My.Windows.MainWindow.SupportedFiles
                    Savedialog.FileName = Editor.FileName
                    If Savedialog.ShowDialog Then
                        Editor.Save(Savedialog.FileName)
                    Else
                        e.Cancel = True
                    End If
                End If
            ElseIf m.Result = "Cancel" Then
                e.Cancel = True
            End If
        End If
    End Sub
End Class