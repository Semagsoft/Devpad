Imports ICSharpCode.AvalonEdit.Document, ICSharpCode.AvalonEdit.Highlighting
Class MainWindow

#Region "Private"

    Private SelectedFile As CodeEditor = Nothing
    Public SupportedFiles As String = _
        "Supported Files(*.asm;*.aspx;*.bat;*.boo;*.cpp;*.h;*.cs;*.css;*;.gml;*.html;*.htm;*.ini;*.java;*.js;*.lua;*.php;*.pl;*.py;*.rb;*.sql*.txt;*.vb;*.xml)|*.asm;*.aspx;*.bat;*.boo;*.cpp;*.h;*.cs;*.gml;*.html;*.htm;*.ini;*.java;*.js;*.lua;*.php;*.pl;*.py;*.rb;*.sql;*.txt;*.vb;*.xml|" + _
        "ASM Files(*.asm)|*.asm|" + _
        "ASP.Net Files(*.aspx)|*.aspx|" + _
        "Boo Files(*.boo)|*.boo|" + _
        "C++ Files(*.cpp;*.h)|*.cpp;*.h|" + _
        "C# Files(*.cs)|*.cs|" + _
        "CSS Files(*.css)|*.css|" + _
        "GML Files(*.gml)|*.gml|" + _
        "HTML Files(*.html;*.htm)|*.html;*.htm|" + _
        "INI Files(*.ini)|*.ini|" + _
        "Java Files(*.java)|*.java|" + _
        "JavaScript Files(*.js)|*.js|" + _
        "Misc(*.bat;*.gml;*.lua;*.pl;*.py;*.rb;*.txt)|*.bat;*.gml;*.lua;*.pl;*.py;*.rb;*.txt|" + _
        "Php Files(*.php)|*.php|" + _
        "SQL Files(*.sql)|*.sql|" + _
        "VB.Net Files(*.vb)|*.vb|" + _
        "Xml Files(*.xml)|*.xml|" + _
        "All Files(*.*)|*.*"

#End Region

#Region "Reusable Code"

#Region "UpdateSyntax"

    Private Sub ClearSyntax()
        For Each i As Object In SyntaxMenuItem.Items
            If TypeName(i) = "MenuItem" Then
                i.IsChecked = False
            ElseIf TypeName(i) = "StackPanel" Then
                Dim stack As StackPanel = i
                For Each i2 As MenuItem In stack.Children
                    i2.IsChecked = False
                Next
            End If
        Next
    End Sub

    Private Sub UpdateSyntax(ext As String)
        ClearSyntax()
        If ext.ToLower = ".asm" Then
            ASMSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".aspx" Then
            ASPSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".boo" Then
            BooSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".cpp" OrElse ext.ToLower = ".h" Then
            CPPSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".cs" Then
            CSSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".css" Then
            CSSSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".gml" Then
            GMLSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".html" OrElse ext.ToLower = ".htm" Then
            HTMLSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".ini" Then
            INISyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".lua" Then
            LUASyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".java" Then
            JavaSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".js" Then
            JavaScriptSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".pl" Then
            PerlSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".php" Then
            PHPSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".sql" Then
            SQLSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".vb" Then
            VBSyntaxMenuItem.IsChecked = True
        ElseIf ext.ToLower = ".xml" Then
            XMLSyntaxMenuItem.IsChecked = True
        Else
            DisableSyntaxMenuItem.IsChecked = True
        End If
    End Sub

#End Region

#Region "UpdateButtons"

    Public Sub UpdateButtons()
        If TabCell.ChildrenCount > 0 AndAlso TabCell.SelectedContent IsNot Nothing Then
            For Each file As FileTab In TabCell.Children
                If file.Editor.FileChanged Then
                    file.Title = file.Editor.FileName + "*"
                Else
                    file.Title = file.Editor.FileName
                End If
            Next
            If SelectedFile.FileChanged Then
                Title = SelectedFile.FileName + "* - " + My.Application.Info.ProductName
            Else
                Title = SelectedFile.FileName + " - " + My.Application.Info.ProductName
            End If

            If SelectedFile.IsModified AndAlso SelectedFile.FileFullName IsNot Nothing Then
                RevertMenuItem.IsEnabled = True
            Else
                RevertMenuItem.IsEnabled = False
            End If
            If SelectedFile.FileFullName IsNot Nothing Then
                RenameMenuItem.IsEnabled = True
                DeleteFileMenuItem.IsEnabled = True
                FilePropertiesMenuItem.IsEnabled = True
                Dim fileinfo As New IO.FileInfo(SelectedFile.FileFullName)
                If fileinfo.IsReadOnly Then
                    ReadOnlyMenuItem.IsChecked = True
                Else
                    ReadOnlyMenuItem.IsChecked = False
                End If
            Else
                RenameMenuItem.IsEnabled = False
                DeleteMenuItem.IsEnabled = False
                FilePropertiesMenuItem.IsEnabled = False
            End If
            CloseMenuItem.IsEnabled = True
            CloseAllButThisMenuItem.IsEnabled = True
            CloseAllMenuItem.IsEnabled = True
            SaveMenuItem.IsEnabled = True
            SaveButton.IsEnabled = True
            SaveAsMenuItem.IsEnabled = True
            SaveAsButton.IsEnabled = True
            SaveCopyMenuItem.IsEnabled = True
            SaveAllMenuItem.IsEnabled = True
            SaveAllButton.IsEnabled = True
            ExportMenuItem.IsEnabled = True
            PageSetupMenuItem.IsEnabled = True
            PrintMenuItem.IsEnabled = True
            PrintButton.IsEnabled = True
            PrintPreviewMenuItem.IsEnabled = True

            EditMenuItem.IsEnabled = True
            If SelectedFile.CanUndo Then
                UndoMenuItem.IsEnabled = True
                UndoButton.IsEnabled = True
                'TODO: SelectedFile.UndoMenuItem.IsEnabled = True
            Else
                UndoMenuItem.IsEnabled = False
                UndoButton.IsEnabled = False
                'TODO: SelectedFile.UndoMenuItem.IsEnabled = False
            End If
            If SelectedFile.CanRedo Then
                RedoMenuItem.IsEnabled = True
                RedoButton.IsEnabled = True
                'TODO: SelectedFile.RedoMenuItem.IsEnabled = True
            Else
                RedoMenuItem.IsEnabled = False
                RedoButton.IsEnabled = False
                'TODO: SelectedFile.RedoMenuItem.IsEnabled = False
            End If
            CutButton.IsEnabled = True
            CopyButton.IsEnabled = True
            PasteButton.IsEnabled = True
            DeleteMenuItem.IsEnabled = True
            DeleteButton.IsEnabled = True
            FindButton.IsEnabled = True
            ReplaceButton.IsEnabled = True
            GoToButton.IsEnabled = True
            ZoomInMenuItem.IsEnabled = True
            ZoomInButton.IsEnabled = True
            If SelectedFile.ZoomLevel >= 0.2 Then
                ZoomOutMenuItem.IsEnabled = True
                ZoomOutButton.IsEnabled = True
            Else
                ZoomOutMenuItem.IsEnabled = False
                ZoomOutButton.IsEnabled = False
            End If
            ZoomResetMenuItem.IsEnabled = True
            ResetZoomButton.IsEnabled = True
            InsertMenuItem.IsEnabled = True
            EncodingMenuItem.IsEnabled = True
            SyntaxMenuItem.IsEnabled = True
            RunMenuItem.IsEnabled = True
            NavigationMenuItem.IsEnabled = True

            LineCountText.Text = "Line: " + SelectedFile.TextArea.Caret.Line.ToString + "\" + SelectedFile.LineCount.ToString + " Column: " + SelectedFile.TextArea.Caret.Column.ToString
            LineCountText.Visibility = Windows.Visibility.Visible
            If SelectedFile.Encoding IsNot Nothing Then
                EncodingStatusText.Text = "Encoding: " + SelectedFile.Encoding.EncodingName.ToString
                EncodingStatusText.Visibility = Windows.Visibility.Visible
            End If
        Else
            Title = My.Application.Info.ProductName

            RevertMenuItem.IsEnabled = False
            RenameMenuItem.IsEnabled = False
            DeleteFileMenuItem.IsEnabled = False
            CloseMenuItem.IsEnabled = False
            CloseAllButThisMenuItem.IsEnabled = False
            CloseAllMenuItem.IsEnabled = False
            SaveMenuItem.IsEnabled = False
            SaveButton.IsEnabled = False
            SaveAsMenuItem.IsEnabled = False
            SaveAsButton.IsEnabled = False
            SaveCopyMenuItem.IsEnabled = False
            SaveAllMenuItem.IsEnabled = False
            SaveAllButton.IsEnabled = False
            ExportMenuItem.IsEnabled = False
            PageSetupMenuItem.IsEnabled = False
            PrintMenuItem.IsEnabled = False
            PrintButton.IsEnabled = False
            PrintPreviewMenuItem.IsEnabled = False
            FilePropertiesMenuItem.IsEnabled = False
            EditMenuItem.IsEnabled = False
            UndoButton.IsEnabled = False
            RedoButton.IsEnabled = False
            CutButton.IsEnabled = False
            CopyButton.IsEnabled = False
            PasteButton.IsEnabled = False
            DeleteButton.IsEnabled = False
            FindButton.IsEnabled = False
            ReplaceButton.IsEnabled = False
            GoToButton.IsEnabled = False

            ZoomInMenuItem.IsEnabled = False
            ZoomInButton.IsEnabled = False
            ZoomOutMenuItem.IsEnabled = False
            ZoomOutButton.IsEnabled = False
            ZoomResetMenuItem.IsEnabled = False
            ResetZoomButton.IsEnabled = False
            InsertMenuItem.IsEnabled = False
            EncodingMenuItem.IsEnabled = False
            SyntaxMenuItem.IsEnabled = False
            RunMenuItem.IsEnabled = False
            NavigationMenuItem.IsEnabled = False

            LineCountText.Visibility = Windows.Visibility.Collapsed
            EncodingStatusText.Visibility = Windows.Visibility.Collapsed
        End If
    End Sub

#End Region

#Region "UpdateUILayout"

    Private Sub UpdateUILayout()
        If MainMenu.Visibility = Windows.Visibility.Visible AndAlso ToolBar1.Visibility = Windows.Visibility.Visible _
            AndAlso StatusBar1.Visibility = Windows.Visibility.Visible Then
            ToolBar1.Margin = New Thickness(-9, 20, -17, 0)
            dockingManager.Margin = New Thickness(-1, 46, 0, 22)
        ElseIf MainMenu.Visibility = Windows.Visibility.Collapsed AndAlso ToolBar1.Visibility = Windows.Visibility.Visible _
            AndAlso StatusBar1.Visibility = Windows.Visibility.Visible Then
            ToolBar1.Margin = New Thickness(-9, 0, -17, 0)
            dockingManager.Margin = New Thickness(-1, 26, 0, 22)
        ElseIf MainMenu.Visibility = Windows.Visibility.Visible AndAlso ToolBar1.Visibility = Windows.Visibility.Collapsed _
            AndAlso StatusBar1.Visibility = Windows.Visibility.Visible Then
            ToolBar1.Margin = New Thickness(-9, 0, -17, 0)
            dockingManager.Margin = New Thickness(-1, 21, 0, 22)
        ElseIf MainMenu.Visibility = Windows.Visibility.Visible AndAlso ToolBar1.Visibility = Windows.Visibility.Visible _
            AndAlso StatusBar1.Visibility = Windows.Visibility.Collapsed Then
            ToolBar1.Margin = New Thickness(-9, 20, -17, 0)
            dockingManager.Margin = New Thickness(-1, 46, 0, 0)
        ElseIf MainMenu.Visibility = Windows.Visibility.Visible AndAlso ToolBar1.Visibility = Windows.Visibility.Collapsed _
            AndAlso StatusBar1.Visibility = Windows.Visibility.Collapsed Then
            dockingManager.Margin = New Thickness(-1, 21, 0, 0)
        ElseIf MainMenu.Visibility = Windows.Visibility.Collapsed AndAlso ToolBar1.Visibility = Windows.Visibility.Visible _
            AndAlso StatusBar1.Visibility = Windows.Visibility.Collapsed Then
            ToolBar1.Margin = New Thickness(-9, 0, -17, 0)
            dockingManager.Margin = New Thickness(-1, 26, 0, 0)
        ElseIf MainMenu.Visibility = Windows.Visibility.Collapsed AndAlso ToolBar1.Visibility = Windows.Visibility.Collapsed _
           AndAlso StatusBar1.Visibility = Windows.Visibility.Visible Then
            dockingManager.Margin = New Thickness(-1, 0, 0, 22)
        ElseIf MainMenu.Visibility = Windows.Visibility.Collapsed AndAlso ToolBar1.Visibility = Windows.Visibility.Collapsed _
            AndAlso StatusBar1.Visibility = Windows.Visibility.Collapsed Then
            dockingManager.Margin = New Thickness(-1, 0, 0, 0)
        End If
    End Sub

#End Region

#Region "SaveFile"

    Private Sub SaveFile(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs)
        Dim header As TabHeader = TryCast(e.Source, TabHeader), file As FileTab = TryCast(header.Parent, FileTab)
        If file.Editor.FileFullName IsNot Nothing Then
            file.Editor.Save(file.Editor.FileFullName)
            file.Editor.FileChanged = False
        Else
            Dim save As New Microsoft.Win32.SaveFileDialog
            save.AddExtension = True
            save.Filter = SupportedFiles
            save.FileName = file.Editor.FileName
            If save.ShowDialog Then
                Dim f As New IO.FileInfo(save.FileName)
                file.Editor.Save(f.FullName)
                file.Editor.FileFullName = f.FullName
                file.Editor.FileName = f.Name
                file.Editor.FileChanged = False
                file.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(f.Extension.ToLower)
                If SelectedFile Is file Then
                    UpdateSyntax(f.Extension)
                End If
            End If
        End If
        UpdateButtons()
    End Sub

#End Region

#Region "SaveFileAs"

    Private Sub SaveFileAs(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs)
        Dim header As TabHeader = TryCast(e.Source, TabHeader), file As FileTab = TryCast(header.Parent, FileTab)
        Dim save As New Microsoft.Win32.SaveFileDialog
        save.AddExtension = True
        save.Filter = SupportedFiles
        save.FileName = file.Editor.FileName
        If save.ShowDialog Then
            Dim f As New IO.FileInfo(save.FileName)
            file.Editor.Save(f.FullName)
            file.Editor.FileFullName = f.FullName
            file.Editor.FileName = f.Name
            file.Editor.FileChanged = False
            file.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(f.Extension.ToLower)
            If SelectedFile Is file Then
                UpdateSyntax(f.Extension)
            End If
        End If
        UpdateButtons()
    End Sub

#End Region

#End Region

#Region "MainWindow"

#Region "Initialized"

    Private Sub MainWindow_Initialized(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Initialized
        Me.AddHandler(TabHeader.SaveEvent, New RoutedEventHandler(AddressOf SaveFile))
        Me.AddHandler(TabHeader.SaveAsEvent, New RoutedEventHandler(AddressOf SaveFileAs))
        Me.AddHandler(TabHeader.SaveAllEvent, New RoutedEventHandler(AddressOf SaveAllMenuItem_Click))
        Me.AddHandler(TabHeader.CloseTabEvent, New RoutedEventHandler(AddressOf CloseFile))
        Me.AddHandler(TabHeader.CloseAllButThisEvent, New RoutedEventHandler(AddressOf CloseAllButThis))
        Me.AddHandler(TabHeader.CloseAllEvent, New RoutedEventHandler(AddressOf CloseAllMenuItem_Click))
        Me.AddHandler(FileTab.FindEvent, New RoutedEventHandler(AddressOf FindMenuItem_Click))
        Me.AddHandler(FileTab.ReplaceEvent, New RoutedEventHandler(AddressOf ReplaceMenuItem_Click))
        Me.AddHandler(FileTab.GoToEvent, New RoutedEventHandler(AddressOf GoToMenuItem_Click))
        Me.AddHandler(FileTab.UpdateSelected, New RoutedEventHandler(AddressOf UpdateButtons))
        Me.AddHandler(CodeEditor.UpdateSelectedText, New RoutedEventHandler(AddressOf UpdateButtons))

        Dim ASMSyntax As IHighlightingDefinition
        Using reader As Xml.XmlReader = Xml.XmlReader.Create(My.Application.Info.DirectoryPath + "\Syntax\asm.xshd")
            ASMSyntax = ICSharpCode.AvalonEdit.Highlighting.Xshd.HighlightingLoader.Load(reader, HighlightingManager.Instance)
        End Using
        Dim ASMEX() As String = {".asm"}
        ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.RegisterHighlighting("ASM", ASMEX, ASMSyntax)
        Dim CSSSyntax As IHighlightingDefinition
        Using reader As Xml.XmlReader = Xml.XmlReader.Create(My.Application.Info.DirectoryPath + "\Syntax\css.xshd")
            CSSSyntax = ICSharpCode.AvalonEdit.Highlighting.Xshd.HighlightingLoader.Load(reader, HighlightingManager.Instance)
        End Using
        Dim CSSEX() As String = {".css"}
        ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.RegisterHighlighting("CSS", CSSEX, CSSsyntax)
        Dim GMLSyntax As IHighlightingDefinition
        Using reader As Xml.XmlReader = Xml.XmlReader.Create(My.Application.Info.DirectoryPath + "\Syntax\gml.xshd")
            GMLSyntax = ICSharpCode.AvalonEdit.Highlighting.Xshd.HighlightingLoader.Load(reader, HighlightingManager.Instance)
        End Using
        Dim GMLEX() As String = {".gml"}
        ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.RegisterHighlighting("GML", GMLEX, GMLsyntax)
        Dim INISyntax As IHighlightingDefinition
        Using reader As Xml.XmlReader = Xml.XmlReader.Create(My.Application.Info.DirectoryPath + "\Syntax\ini.xshd")
            INISyntax = ICSharpCode.AvalonEdit.Highlighting.Xshd.HighlightingLoader.Load(reader, HighlightingManager.Instance)
        End Using
        Dim INIEX() As String = {".ini"}
        ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.RegisterHighlighting("INI", INIEX, INISyntax)
        Dim LUASyntax As IHighlightingDefinition
        Using reader As Xml.XmlReader = Xml.XmlReader.Create(My.Application.Info.DirectoryPath + "\Syntax\lua.xshd")
            LUASyntax = ICSharpCode.AvalonEdit.Highlighting.Xshd.HighlightingLoader.Load(reader, HighlightingManager.Instance)
        End Using
        Dim LUAEX() As String = {".lua"}
        ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.RegisterHighlighting("LUA", LUAEX, LUASyntax)
        Dim PERLSyntax As IHighlightingDefinition
        Using reader As Xml.XmlReader = Xml.XmlReader.Create(My.Application.Info.DirectoryPath + "\Syntax\perl.xshd")
            PERLSyntax = ICSharpCode.AvalonEdit.Highlighting.Xshd.HighlightingLoader.Load(reader, HighlightingManager.Instance)
        End Using
        Dim PERLEX() As String = {".pl"}
        ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.RegisterHighlighting("PERL", PERLEX, PERLSyntax)
        Dim SQLSyntax As IHighlightingDefinition
        Using reader As Xml.XmlReader = Xml.XmlReader.Create(My.Application.Info.DirectoryPath + "\Syntax\sql.xshd")
            SQLSyntax = ICSharpCode.AvalonEdit.Highlighting.Xshd.HighlightingLoader.Load(reader, HighlightingManager.Instance)
        End Using
        Dim SQLEX() As String = {".sql"}
        ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.RegisterHighlighting("SQL", SQLEX, SQLsyntax)
    End Sub

#End Region

#Region "Loaded"

    Private Sub MainWindow_Loaded(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles Me.Loaded
        If My.Computer.Info.OSVersion >= "6.0" Then
            AppHelper.ExtendGlassFrame(Me, New Thickness(-1, -1, -1, -1))
        End If

        Dim b As SolidColorBrush = Nothing
        If My.Settings.Options_Theme = 0 Then
            b = New SolidColorBrush(Color.FromRgb(0, 0, 0))
            dockingManager.Theme = New Xceed.Wpf.AvalonDock.Themes.AeroTheme
        ElseIf My.Settings.Options_Theme = 1 Then
            b = New SolidColorBrush(Color.FromRgb(123, 123, 123))
            dockingManager.Theme = New Xceed.Wpf.AvalonDock.Themes.ExpressionDarkTheme
        ElseIf My.Settings.Options_Theme = 2 Then
            b = New SolidColorBrush(Color.FromRgb(255, 255, 255))
            dockingManager.Theme = New Xceed.Wpf.AvalonDock.Themes.ExpressionLightTheme
        ElseIf My.Settings.Options_Theme = 3 Then
            b = New SolidColorBrush(Color.FromRgb(255, 255, 255))
        End If
        b.Opacity = 0.5
        BackgroundGrid.Background = b

        If My.Settings.MainWindow_IsMax Then
            WindowState = Windows.WindowState.Maximized
        Else
            Height = My.Settings.MainWindow_Size.Height
            Width = My.Settings.MainWindow_Size.Width
            Left = My.Settings.MainWindow_Loc.X
            Top = My.Settings.MainWindow_Loc.Y
        End If

        If My.Settings.MainWindow_ShowMenubar Then
            MenubarMenuItem.IsChecked = True
        Else
            MenubarMenuItem.IsChecked = False
            MainMenu.Visibility = Windows.Visibility.Collapsed
        End If
        If My.Settings.MainWindow_ShowToolbar Then
            ToolbarMenuItem.IsChecked = True
        Else
            ToolbarMenuItem.IsChecked = False
            ToolBar1.Visibility = Windows.Visibility.Collapsed
        End If
        If My.Settings.MainWindow_ShowStatusbar Then
            StatusMenuItem.IsChecked = True
        Else
            StatusMenuItem.IsChecked = False
            StatusBar1.Visibility = Windows.Visibility.Collapsed
        End If
        UpdateUILayout()

        If My.Application.StartUpFileNames.Count > 0 Then
            For Each s As String In My.Application.StartUpFileNames
                Dim fileinfo As New IO.FileInfo(s)
                Dim t As New FileTab(Me)
                t.Title = fileinfo.Name
                t.Editor.FileFullName = fileinfo.FullName
                t.Editor.FileName = fileinfo.Name
                t.Editor.Load(fileinfo.FullName)
                t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(fileinfo.Extension.ToLower)
                UpdateSyntax(fileinfo.Extension)
                t.Editor.FileChanged = False
                TabCell.Children.Add(t)
                If fileinfo.IsReadOnly Then
                    ReadOnlyMenuItem.IsChecked = True
                    t.Editor.IsReadOnly = True
                Else
                    ReadOnlyMenuItem.IsChecked = False
                    t.Editor.IsReadOnly = False
                End If
                t.IsSelected = True
            Next
        Else
            If My.Settings.Options_StartupMode = 0 Then
                Dim t As New FileTab(Me)
                TabCell.Children.Add(t)
                If My.Settings.Options_DefaultFormat = 0 Then
                    t.Title = "newfile.txt"
                    t.Editor.FileName = "newfile.txt"
                ElseIf My.Settings.Options_DefaultFormat = 1 Then
                    t.Title = "newfile.aspx"
                    t.Editor.FileName = "newfile.aspx"
                    t.Editor.FileFormat = "ASP.Net"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("ASP/XHTML")
                ElseIf My.Settings.Options_DefaultFormat = 2 Then
                    t.Title = "newfile.boo"
                    t.Editor.FileName = "newfile.boo"
                    t.Editor.FileFormat = "Boo"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("Boo")
                ElseIf My.Settings.Options_DefaultFormat = 3 Then
                    t.Title = "newfile.cs"
                    t.Editor.FileName = "newfile.cs"
                    t.Editor.FileFormat = "C#"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("C#")
                ElseIf My.Settings.Options_DefaultFormat = 4 Then
                    t.Title = "newfile.cpp"
                    t.Editor.FileName = "newfile.cpp"
                    t.Editor.FileFormat = "C++"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("C++")
                ElseIf My.Settings.Options_DefaultFormat = 5 Then
                    t.Title = "newfile.html"
                    t.Editor.FileName = "newfile.html"
                    t.Editor.FileFormat = "HTML"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("HTML")
                ElseIf My.Settings.Options_DefaultFormat = 6 Then
                    t.Title = "newfile.java"
                    t.Editor.FileName = "newfile.java"
                    t.Editor.FileFormat = "Java"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("Java")
                ElseIf My.Settings.Options_DefaultFormat = 7 Then
                    t.Title = "newfile.js"
                    t.Editor.FileName = "newfile.js"
                    t.Editor.FileFormat = "JavaScript"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("JavaScript")
                ElseIf My.Settings.Options_DefaultFormat = 8 Then
                    t.Title = "newfile.php"
                    t.Editor.FileName = "newfile.php"
                    t.Editor.FileFormat = "PHP"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("PHP")
                ElseIf My.Settings.Options_DefaultFormat = 9 Then
                    t.Title = "newfile.vb"
                    t.Editor.FileName = "newfile.vb"
                    t.Editor.FileFormat = "VB"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("VBNET")
                ElseIf My.Settings.Options_DefaultFormat = 10 Then
                    t.Title = "newfile.xml"
                    t.Editor.FileName = "newfile.xml"
                    t.Editor.FileFormat = "XML"
                    t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("XML")
                End If
                t.IsSelected = True
                UpdateButtons()
            ElseIf My.Settings.Options_StartupMode = 1 Then
                NewMenuItem_Click(Nothing, Nothing)
            ElseIf My.Settings.Options_StartupMode = 2 Then
                OpenMenuItem_Click(Nothing, Nothing)
            End If
        End If
        For Each s As String In My.Settings.Options_RecentFiles
            If My.Computer.FileSystem.FileExists(s) Then
                Dim m As New MenuItem, f As New IO.FileInfo(s)
                Dim ContMenu As New ContextMenu
                Dim removemenuitem As New MenuItem
                removemenuitem.Header = "Remove"
                removemenuitem.ToolTip = f.FullName
                ContMenu.Items.Add(removemenuitem)
                m.ContextMenu = ContMenu
                m.Header = f.Name
                m.ToolTip = f.FullName
                RecentMenuItem.Items.Add(m)
                AddHandler (m.Click), New RoutedEventHandler(AddressOf recentfile_click)
                AddHandler removemenuitem.Click, New RoutedEventHandler(AddressOf recentfileremove_click)
            End If
        Next
        If RecentMenuItem.ContextMenu IsNot Nothing Then
            If RecentMenuItem.ContextMenu.Items.Count = 0 Then
                RecentMenuItem.IsEnabled = False
            End If
        End If
        For Each s As String In My.Settings.ExTools
            Dim menuitem As New MenuItem
            Dim f As New IO.FileInfo(s)
            menuitem.Header = f.Name.Remove(f.Name.Length - f.Extension.Length)
            menuitem.ToolTip = f.FullName
            ToolsMenuItem.Items.Insert(0, menuitem)
            AddHandler menuitem.Click, New RoutedEventHandler(AddressOf RunExTool)
        Next
        UpdateButtons()

        Dim ad As New AdWindow
        ad.ShowDialog()
    End Sub

#End Region

#Region "Closing"

    Private Sub MainWindow_Closing(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs) Handles Me.Closing
        While TabCell.ChildrenCount > 0
            If SelectedFile.FileChanged Then
                Dim m As New MessageBoxDialog("Do you want to save " + SelectedFile.FileName + "?", "Save", "YesNoCancel", Nothing)
                m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
                m.Owner = Me
                m.ShowDialog()
                If m.Result = "Yes" Then
                    If SelectedFile.FileFullName IsNot Nothing Then
                        SelectedFile.Save(SelectedFile.FileFullName)
                        dockingManager.Layout.ActiveContent.Close()
                    Else
                        Dim s As New Microsoft.Win32.SaveFileDialog
                        s.AddExtension = True
                        s.Filter = SupportedFiles
                        s.FileName = SelectedFile.FileName
                        If s.ShowDialog Then
                            SelectedFile.Save(s.FileName)
                            dockingManager.Layout.ActiveContent.Close()
                        End If
                    End If
                ElseIf m.Result = "No" Then
                    dockingManager.Layout.ActiveContent.Close()
                ElseIf m.Result = "Cancel" Then
                    e.Cancel = True
                    Exit While
                End If
            Else
                dockingManager.Layout.ActiveContent.Close()
            End If
        End While

        If MainMenu.Visibility = Windows.Visibility.Visible Then
            My.Settings.MainWindow_ShowMenubar = True
        Else
            My.Settings.MainWindow_ShowMenubar = False
        End If
        If ToolBar1.Visibility = Windows.Visibility.Visible Then
            My.Settings.MainWindow_ShowToolbar = True
        Else
            My.Settings.MainWindow_ShowToolbar = False
        End If
        If StatusBar1.Visibility = Windows.Visibility.Visible Then
            My.Settings.MainWindow_ShowStatusbar = True
        Else
            My.Settings.MainWindow_ShowStatusbar = False
        End If

        If WindowState = Windows.WindowState.Maximized Then
            My.Settings.MainWindow_IsMax = True
        ElseIf WindowState = Windows.WindowState.Normal Then
            My.Settings.MainWindow_IsMax = False
            My.Settings.MainWindow_Loc = New System.Drawing.Point(Convert.ToInt32(Left), Convert.ToInt32(Top))
            My.Settings.MainWindow_Size = New System.Drawing.Size(Convert.ToInt32(Width), Convert.ToInt32(Height))
        End If
        My.Settings.Save()
    End Sub

#End Region

#Region "KeyDown"

    Private Sub MainWindow_KeyDown(ByVal sender As Object, ByVal e As System.Windows.Input.KeyEventArgs) Handles Me.KeyDown
        If TabCell.Children.Count > 0 Then
            If Keyboard.IsKeyDown(Key.LeftCtrl) AndAlso Not Keyboard.IsKeyDown(Key.LeftAlt) AndAlso Not Keyboard.IsKeyDown(Key.LeftShift) Then
                If e.Key = Key.N Then
                    NewMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.O Then
                    OpenMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.W Then
                    CloseMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.S Then
                    SaveMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.P Then
                    PrintMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.F Then
                    FindMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.H Then
                    ReplaceMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.G Then
                    GoToMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.OemPlus Then
                    ZoomInMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.OemMinus Then
                    If SelectedFile.ZoomLevel >= 0.2 Then
                        ZoomOutMenuItem_Click(Nothing, Nothing)
                    End If
                    e.Handled = True
                ElseIf e.Key = Key.D0 Then
                    ZoomResetMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                End If
            ElseIf Keyboard.IsKeyDown(Key.LeftCtrl) AndAlso Keyboard.IsKeyDown(Key.LeftShift) AndAlso Not Keyboard.IsKeyDown(Key.LeftAlt) Then
                If e.Key = Key.W Then
                    CloseAllMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.S Then
                    SaveAllMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                End If
            ElseIf Keyboard.IsKeyDown(Key.LeftCtrl) AndAlso Keyboard.IsKeyDown(Key.LeftAlt) AndAlso Not Keyboard.IsKeyDown(Key.LeftShift) Then
                If e.Key = Key.P Then
                    PrintPreviewMenuItem_Click(Nothing, Nothing)
                ElseIf e.Key = Key.W Then
                    CloseAllButThisMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.S Then
                    SaveAsMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                End If
            End If
        Else
            If Keyboard.IsKeyDown(Key.LeftCtrl) Then
                If e.Key = Key.N Then
                    NewMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                ElseIf e.Key = Key.O Then
                    OpenMenuItem_Click(Nothing, Nothing)
                    e.Handled = True
                End If
            End If
        End If
        If Keyboard.IsKeyDown(Key.RightAlt) Then
            MenubarMenuItem_Click(Nothing, Nothing)
            e.Handled = True
        End If
        If Keyboard.IsKeyDown(Key.F11) Then
            FullScreenMenuItem_Click(Nothing, Nothing)
            e.Handled = True
        End If
    End Sub

#End Region

#End Region

#Region "MainMenu"

#Region "File"

#Region "New"

    Private Sub NewMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles NewMenuItem.Click, NewButton.Click
        Dim d As New NewFileDialog
        d.Owner = Me
        d.ShowDialog()
        If d.Res = "OK" Then
            Dim t As New FileTab(Me)
            t.Title = d.TextBox1.Text
            t.Editor.FileName = d.TextBox1.Text
            Dim selected As TreeViewItem = TryCast(d.TreeView1.SelectedItem, TreeViewItem)
            Dim filetypeitem As ListBoxItem = TryCast(d.ListBox1.SelectedItem, ListBoxItem)
            Dim f As New IO.FileInfo(t.Editor.FileName)
            t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(f.Extension.ToLower)
            UpdateSyntax(f.Extension)
            TabCell.Children.Add(t)
            t.IsSelected = True
        End If
    End Sub

#End Region

#Region "Open"

    Private Sub OpenMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles OpenMenuItem.Click, OpenButton.Click
        Dim open As New Microsoft.Win32.OpenFileDialog
        open.Filter = SupportedFiles
        If open.ShowDialog Then
            Dim fileinfo As New IO.FileInfo(open.FileName)
            Dim t As New FileTab(Me)
            t.Title = fileinfo.Name
            t.Editor.FileFullName = fileinfo.FullName
            t.Editor.FileName = fileinfo.Name
            t.Editor.Load(fileinfo.FullName)
            If Not My.Settings.Options_RecentFiles.Contains(fileinfo.FullName) Then
                My.Settings.Options_RecentFiles.Add(fileinfo.FullName)
            End If
            t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(fileinfo.Extension.ToLower)
            UpdateSyntax(fileinfo.Extension)
            t.Editor.FileChanged = False
            TabCell.Children.Add(t)
            If fileinfo.IsReadOnly Then
                ReadOnlyMenuItem.IsChecked = True
                t.Editor.IsReadOnly = True
            Else
                ReadOnlyMenuItem.IsChecked = False
                t.Editor.IsReadOnly = False
            End If
            t.IsSelected = True
        End If
    End Sub

#End Region

#Region "RecentFiles"

    Private Sub recentfile_click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs)
        Dim i As MenuItem = TryCast(e.Source, MenuItem), f As New IO.FileInfo(TryCast(i.ToolTip, String))
        Dim t As New FileTab(Me)
        t.Title = f.Name
        t.Editor.FileFullName = f.FullName
        t.Editor.FileName = f.Name
        t.Editor.Load(f.FullName)
        t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(f.Extension.ToLower)
        UpdateSyntax(f.Extension)
        t.Editor.FileChanged = False
        TabCell.Children.Add(t)
        If f.IsReadOnly Then
            ReadOnlyMenuItem.IsChecked = True
            t.Editor.IsReadOnly = True
        Else
            ReadOnlyMenuItem.IsChecked = False
            t.Editor.IsReadOnly = False
        End If
        t.IsSelected = True
    End Sub

    Private Sub recentfileremove_click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs)
        Dim i As MenuItem = e.Source
        Dim itemtoremove As MenuItem = Nothing
        For Each recentdoc As MenuItem In RecentMenuItem.Items
            If recentdoc.ToolTip = i.ToolTip Then
                itemtoremove = recentdoc
            End If
        Next
        Dim stringtoremove As String = Nothing
        For Each s As String In My.Settings.Options_RecentFiles
            If s = i.ToolTip Then
                stringtoremove = s
            End If
        Next
        My.Settings.Options_RecentFiles.Remove(stringtoremove)
        RecentMenuItem.Items.Remove(itemtoremove)
    End Sub

#End Region

#Region "Revert"

    Private Sub RevertMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles RevertMenuItem.Click
        SelectedFile.Load(SelectedFile.FileFullName)
        SelectedFile.FileChanged = False
        UpdateButtons()
    End Sub

#End Region

#Region "Rename"

    Private Sub RenameMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles RenameMenuItem.Click
        Dim rename As New Microsoft.Win32.SaveFileDialog
        rename.Filter = SupportedFiles
        If rename.ShowDialog = True Then
            My.Computer.FileSystem.CopyFile(SelectedFile.FileFullName, rename.FileName, True)
            My.Computer.FileSystem.DeleteFile(SelectedFile.FileFullName)
            SelectedFile.FileFullName = rename.FileName
            Dim file As New IO.FileInfo(rename.FileName)
            SelectedFile.FileName = file.Name
            dockingManager.Layout.ActiveContent.Title = file.Name
            Dim f As New IO.FileInfo(SelectedFile.FileName)
            SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(f.Extension.ToLower)
            UpdateSyntax(f.Extension)
            UpdateButtons()
        End If
    End Sub

#End Region

#Region "Delete"

    Private Sub DeleteFileMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles DeleteFileMenuItem.Click
        Dim m As New MessageBoxDialog("The file """ + SelectedFile.FileFullName + """ will be deleted from your disk and this file will be closed. Continue?", "Delete File", "YesNo", Nothing)
        m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
        m.Owner = Me
        m.ShowDialog()
        If m.Result = "Yes" Then
            My.Computer.FileSystem.DeleteFile(SelectedFile.FileFullName)
            dockingManager.Layout.ActiveContent.Close()
            UpdateButtons()
        End If
    End Sub

#End Region

#Region "Close"

    Private Sub CloseMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CloseMenuItem.Click
        If SelectedFile.FileChanged Then
            Dim m As New MessageBoxDialog("Do you want to save " + SelectedFile.FileName + "?", "Save", "YesNoCancel", Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
            m.Owner = Me
            m.ShowDialog()
            If m.Result = "Yes" Then
                If SelectedFile.FileFullName IsNot Nothing Then
                    SelectedFile.Save(SelectedFile.FileFullName)
                    dockingManager.Layout.ActiveContent.Close()
                Else
                    Dim d As New Microsoft.Win32.SaveFileDialog
                    d.AddExtension = True
                    d.Filter = SupportedFiles
                    d.FileName = SelectedFile.FileName
                    If d.ShowDialog Then
                        Dim file As New IO.FileInfo(d.FileName)
                        SelectedFile.Save(d.FileName)
                        dockingManager.Layout.ActiveContent.Close()
                    End If
                End If
            ElseIf m.Result = "No" Then
                dockingManager.Layout.ActiveContent.Close()
            End If
        Else
            dockingManager.Layout.ActiveContent.Close()
        End If
    End Sub

#End Region

#Region "Close All But This"

    Private Sub CloseAllButThisMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CloseAllButThisMenuItem.Click
        While TabCell.ChildrenCount > 1
            If TabCell.Children.Item(0) IsNot SelectedFile Then
                Dim t As FileTab = TryCast(TabCell.Children.Item(0), FileTab)
                If t.Editor.FileChanged Then
                    Dim m As New MessageBoxDialog("Do you want to save " + t.Editor.FileName + "?", "Save", "YesNoCancel", Nothing)
                    m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
                    m.Owner = Me
                    If m.Result = MessageBoxResult.Yes Then
                        If t.Editor.FileFullName IsNot Nothing Then
                            t.Editor.Save(t.Editor.FileFullName)
                            TabCell.Children.RemoveAt(0)
                        Else
                            Dim d As New Microsoft.Win32.SaveFileDialog
                            d.Filter = SupportedFiles
                            If d.ShowDialog Then
                                Dim file As New IO.FileInfo(d.FileName)
                                t.Editor.Save(d.FileName)
                                TabCell.Children.RemoveAt(0)
                            End If
                        End If
                    ElseIf m.Result = MessageBoxResult.No Then
                        TabCell.Children.RemoveAt(0)
                    Else
                        Exit While
                    End If
                Else
                    TabCell.Children.RemoveAt(0)
                End If
            Else
                Dim t As FileTab = TryCast(TabCell.Children.Item(1), FileTab)
                If t.Editor.FileChanged Then
                    Dim m As New MessageBoxDialog("Do you want to save " + t.Editor.FileName + "?", "Save", "YesNoCancel", Nothing)
                    m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
                    m.Owner = Me
                    If m.Result = "Yes" Then
                        If t.Editor.FileFullName IsNot Nothing Then
                            t.Editor.Save(t.Editor.FileFullName)
                            TabCell.Children.RemoveAt(1)
                        Else
                            Dim d As New Microsoft.Win32.SaveFileDialog
                            d.AddExtension = True
                            d.Filter = SupportedFiles
                            d.FileName = t.Editor.FileName
                            If d.ShowDialog Then
                                Dim file As New IO.FileInfo(d.FileName)
                                t.Editor.Save(d.FileName)
                                TabCell.Children.RemoveAt(1)
                            End If
                        End If
                    ElseIf m.Result = "No" Then
                        TabCell.Children.RemoveAt(1)
                    Else
                        Exit While
                    End If
                Else
                    TabCell.Children.RemoveAt(1)
                End If
            End If
        End While
    End Sub

#End Region

#Region "Close All"

    Private Sub CloseAllMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CloseAllMenuItem.Click
        While TabCell.Children.Count > 0
            If SelectedFile.FileChanged Then
                Dim m As New MessageBoxDialog("Do you want to save " + SelectedFile.FileName + "?", "Save", "YesNoCancel", Nothing)
                m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
                m.Owner = Me
                If m.Result = "Yes" Then
                    If SelectedFile.FileFullName IsNot Nothing Then
                        SelectedFile.Save(SelectedFile.FileFullName)
                        dockingManager.Layout.ActiveContent.Close()
                    Else
                        Dim d As New Microsoft.Win32.SaveFileDialog
                        d.AddExtension = True
                        d.Filter = SupportedFiles
                        d.FileName = SelectedFile.FileName
                        If d.ShowDialog Then
                            Dim file As New IO.FileInfo(d.FileName)
                            SelectedFile.Save(d.FileName)
                            dockingManager.Layout.ActiveContent.Close()
                        End If
                    End If
                ElseIf m.Result = "No" Then
                    dockingManager.Layout.ActiveContent.Close()
                Else
                    Exit While
                End If
            Else
                dockingManager.Layout.ActiveContent.Close()
            End If
        End While
    End Sub

#End Region

#Region "Save"

    Private Sub SaveMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles SaveMenuItem.Click, SaveButton.Click
        If SelectedFile.FileFullName IsNot Nothing Then
            SelectedFile.Save(SelectedFile.FileFullName)
            SelectedFile.FileChanged = False
        Else
            Dim d As New Microsoft.Win32.SaveFileDialog
            d.Title = "Save"
            d.AddExtension = True
            d.Filter = SupportedFiles
            d.FileName = SelectedFile.FileName
            If d.ShowDialog Then
                Dim file As New IO.FileInfo(d.FileName)
                SelectedFile.Save(file.FullName)
                SelectedFile.FileFullName = file.FullName
                SelectedFile.FileName = file.Name
                dockingManager.Layout.ActiveContent.Title = file.Name
                SelectedFile.FileChanged = False
                Dim f As New IO.FileInfo(SelectedFile.FileName)
                SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(f.Extension.ToLower)
                UpdateSyntax(f.Extension)
            End If
        End If
        UpdateButtons()
    End Sub

#End Region

#Region "Save As"

    Private Sub SaveAsMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles SaveAsMenuItem.Click, SaveAsButton.Click
        Dim d As New Microsoft.Win32.SaveFileDialog
        d.Title = "Save As"
        d.AddExtension = True
        d.Filter = SupportedFiles
        d.FileName = SelectedFile.FileName
        If d.ShowDialog Then
            Dim file As New IO.FileInfo(d.FileName)
            SelectedFile.Save(file.FullName)
            SelectedFile.FileFullName = file.FullName
            SelectedFile.FileName = file.Name
            dockingManager.Layout.ActiveContent.Title = file.Name
            SelectedFile.FileChanged = False
            Dim f As New IO.FileInfo(SelectedFile.FileName)
            SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(f.Extension.ToLower)
            UpdateSyntax(f.Extension)
        End If
        UpdateButtons()
    End Sub

#End Region

#Region "Save Copy"

    Private Sub SaveCopyMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles SaveCopyMenuItem.Click
        Dim save As New Microsoft.Win32.SaveFileDialog
        save.Title = "Save Copy"
        save.Filter = SupportedFiles
        If save.ShowDialog Then
            SelectedFile.Save(save.FileName)
        End If
    End Sub

#End Region

#Region "Save All"

    Private Sub SaveAllMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles SaveAllMenuItem.Click, SaveAllButton.Click
        For Each t As FileTab In TabCell.Children
            If t.Editor.FileChanged Then
                If t.Editor.FileFullName IsNot Nothing Then
                    t.Editor.Save(t.Editor.FileFullName)
                    t.Editor.FileChanged = False
                Else
                    Dim save As New Microsoft.Win32.SaveFileDialog
                    save.AddExtension = True
                    save.Filter = SupportedFiles
                    save.FileName = t.Editor.FileName
                    If save.ShowDialog Then
                        Dim f As New IO.FileInfo(save.FileName)
                        t.Editor.Save(f.FullName)
                        t.Title = f.Name
                        t.Editor.FileFullName = f.FullName
                        t.Editor.FileName = f.Name
                        t.Editor.FileChanged = False
                        t.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(f.Extension.ToLower)
                        UpdateSyntax(f.Extension)
                    End If
                End If
            End If
        Next
    End Sub

#End Region

#Region "Import"

#Region "FTP"

    Private Sub ImportFTPMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles ImportFTPMenuItem.Click
        Dim import As New ImportFTPDialog
        import.Owner = Me
        import.ShowDialog()
        If import.Res = True Then
            Dim tab As New FileTab(Me), f As New IO.FileInfo(import.FileToLoad)
            TabCell.Children.Add(tab)
            tab.IsSelected = True
            SelectedFile.Load(import.FileToLoad)
            SelectedFile.FileName = "newfile" + f.Extension
            SelectedFile.FileFullName = Nothing
            SelectedFile.FileChanged = False
            dockingManager.Layout.ActiveContent.Title = "newfile" + f.Extension
            SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(f.Extension)
            UpdateSyntax(f.Extension)
            My.Computer.FileSystem.DeleteFile(import.FileToLoad)
            UpdateButtons()
        End If
    End Sub

#End Region

#Region "Archive"

    Private Sub ImportArchiveMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles ImportArchiveMenuItem.Click
        Dim import As New Microsoft.Win32.OpenFileDialog
        import.Title = "Import Archive"
        import.Filter = "Zip Archive(*.zip)|*.zip|All Files(*.*)|*.*"
        If import.ShowDialog Then
            Dim zip As New Ionic.Zip.ZipFile(import.FileName)
            For Each item As Ionic.Zip.ZipEntry In zip.Entries
                item.Extract(My.Computer.FileSystem.SpecialDirectories.CurrentUserApplicationData + "\Semagsoft", Ionic.Zip.ExtractExistingFileAction.OverwriteSilently)
                Dim tab As New FileTab(Me)
                Dim file As New IO.FileInfo(My.Computer.FileSystem.SpecialDirectories.CurrentUserApplicationData + "\Semagsoft\" + item.FileName)
                tab.Editor.FileFullName = file.FullName
                tab.Editor.FileName = item.FileName
                tab.Editor.Load(file.FullName)
                tab.Editor.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinitionByExtension(file.Extension.ToLower)
                UpdateSyntax(file.Extension)
                tab.Editor.FileChanged = False
                TabCell.Children.Add(tab)
                tab.IsSelected = True
            Next
        End If
    End Sub

#End Region

#Region "FlowDocument"

    Private Sub ImportFlowDocumentMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles ImportFlowDocumentMenuItem.Click
        Dim import As New Microsoft.Win32.OpenFileDialog
        import.Title = "Import FlowDocument"
        import.Filter = "FlowDocuments(*.xaml)|*.xaml|All Files(*.*)|*.*"
        If import.ShowDialog Then
            Dim f As New IO.FileInfo(import.FileName), fs As IO.FileStream = IO.File.Open(f.FullName, IO.FileMode.Open, IO.FileAccess.Read)
            Dim content As FlowDocument = TryCast(Markup.XamlReader.Load(fs), FlowDocument)
            Dim tr As New TextRange(content.ContentStart, content.ContentEnd)
            Dim tab As New FileTab(Me)
            tab.Editor.FileName = f.Name.Remove(f.Name.Length - f.Extension.Length) + ".txt"
            tab.Editor.Text = tr.Text
            tab.Editor.FileChanged = False
            fs.Close()
            TabCell.Children.Add(tab)
            tab.IsSelected = True
        End If
    End Sub

#End Region

#Region "Rich Text"

    Private Sub ImportRichTextMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles ImportRichTextMenuItem.Click
        Dim import As New Microsoft.Win32.OpenFileDialog
        import.Title = "Import Rich Text"
        import.Filter = "Rich Text Files(*.rtf)|*.rtf|All Files(*.*)|*.*"
        If import.ShowDialog Then
            Dim f As New IO.FileInfo(import.FileName), fs As IO.FileStream = IO.File.Open(f.FullName, IO.FileMode.Open)
            Dim tab As New FileTab(Me)
            Dim rtf As New RichTextBox
            Dim tr As New TextRange(rtf.Document.ContentStart, rtf.Document.ContentEnd)
            tr.Load(fs, Windows.DataFormats.Rtf)
            tab.Editor.FileName = f.Name.Remove(f.Name.Length - f.Extension.Length) + ".txt"
            tab.Editor.Text = tr.Text
            tab.Editor.FileChanged = False
            fs.Close()
            TabCell.Children.Add(tab)
            tab.IsSelected = True
        End If
    End Sub

#End Region

#End Region

#Region "Export"

#Region "FTP"

    Private Sub ExportFTPMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles ExportFTPMenuItem.Click
        Dim d As New ExportFTPDialog
        d.Owner = Me
        If SelectedFile.FileFullName IsNot Nothing Then
            d.FileName = SelectedFile.FileFullName
            d.ShowDialog()
        Else
            If Not My.Computer.FileSystem.DirectoryExists(My.Application.Info.DirectoryPath + "\Temp") Then
                My.Computer.FileSystem.CreateDirectory(My.Application.Info.DirectoryPath + "\Temp")
            End If
            Dim file As New IO.FileInfo(My.Application.Info.DirectoryPath + "\Temp\" + SelectedFile.FileName), fs As IO.FileStream = IO.File.Open(file.FullName, IO.FileMode.OpenOrCreate)
            fs.Close()
            SelectedFile.Save(file.FullName)
            d.FileName = file.FullName
            d.ShowDialog()
        End If
    End Sub

#End Region

#Region "Email"

    Private Sub EmailMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles EmailMenuItem.Click
        Dim email As New EmailDialog
        email.Owner = Me
        email.ShowDialog()
        If email.Res = "OK" Then
            Dim SmtpServer As New Net.Mail.SmtpClient()
            SmtpServer.Credentials = New Net.NetworkCredential(email.FromBox.Text, email.EmailPassword.Password)
            SmtpServer.Port = Convert.ToInt32(email.PortTextBox.Text)
            SmtpServer.Host = email.SMTPComboBox.Text
            SmtpServer.EnableSsl = True

            Dim mail As New Net.Mail.MailMessage()
            Dim addr() As String = email.ToBox.Text.Split(",")
            Try
                mail.From = New Net.Mail.MailAddress(email.FromBox.Text, email.FromBox.Text, System.Text.Encoding.UTF8)

                Dim i As Byte
                For i = 0 To addr.Length - 1
                    mail.To.Add(addr(i))
                Next
                mail.Subject = email.SubjectBox.Text
                mail.Body = email.BodyBox.Text
                Dim attach As Net.Mail.Attachment
                If SelectedFile.FileFullName IsNot Nothing Then
                    attach = New System.Net.Mail.Attachment(SelectedFile.FileFullName)
                Else
                    If Not My.Computer.FileSystem.DirectoryExists(My.Application.Info.DirectoryPath + "\Temp") Then
                        My.Computer.FileSystem.CreateDirectory(My.Application.Info.DirectoryPath + "\Temp")
                    End If
                    Dim file As New IO.FileInfo(My.Application.Info.DirectoryPath + "\Temp\" + SelectedFile.FileName), fs As IO.FileStream = IO.File.Open(file.FullName, IO.FileMode.OpenOrCreate)
                    SelectedFile.Save(fs)
                    fs.Close()
                    attach = New System.Net.Mail.Attachment(file.FullName)
                End If
                mail.Attachments.Add(attach)
                mail.DeliveryNotificationOptions = Net.Mail.DeliveryNotificationOptions.OnFailure
                SmtpServer.Send(mail)
                Dim m As New MessageBoxDialog("Message Sent", "Export", Nothing, Nothing)
                m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/info32.png"))
                m.Owner = Me
                m.ShowDialog()
            Catch ex As Exception
                Dim m As New MessageBoxDialog(ex.Message, "Error", Nothing, Nothing)
                m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/error32.png"))
                m.Owner = Me
                m.ShowDialog()
            End Try
        End If
    End Sub

#End Region

#Region "Wordpress"

    Private Sub ExportWordpressMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles ExportWordpressMenuItem.Click
        Dim d As New WordpressDialog
        d.Owner = Me
        d.ShowDialog()
        If d.Res = "OK" Then
            Dim newBlogPost As AppHelper.blogInfo = Nothing
            newBlogPost.title = d.TitleTextBox.Text
            newBlogPost.description = SelectedFile.Text
            Dim categories As AppHelper.IgetCatList = CType(CookComputing.XmlRpc.XmlRpcProxyGen.Create(GetType(AppHelper.IgetCatList)), AppHelper.IgetCatList)
            Dim clientProtocol As CookComputing.XmlRpc.XmlRpcClientProtocol = CType(categories, CookComputing.XmlRpc.XmlRpcClientProtocol)
            clientProtocol.Url = d.BlogTextBox.Text + "/xmlrpc.php"
            Dim result As String = Nothing
            result = ""
            Try
                result = categories.NewPage(1, d.UserNameTextBox.Text, d.PasswordTextBox.Password, newBlogPost, 1)
                Dim m As New MessageBoxDialog("Posted to Blog successfullly! Post ID : " & result, "Export", Nothing, Nothing)
                m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/info32.png"))
                m.Owner = Me
                m.ShowDialog()
                MessageBox.Show("Posted to Blog successfullly! Post ID : " & result)
            Catch ex As Exception
                Dim m As New MessageBoxDialog(ex.Message, "Error", Nothing, Nothing)
                m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/error32.png"))
                m.Owner = Me
                m.ShowDialog()
            End Try
        End If
    End Sub

#End Region

#Region "Export Archive"

    Private Sub ExportArchiveMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles ExportArchiveMenuItem.Click
        Dim export As New Microsoft.Win32.SaveFileDialog
        export.Title = "Export Archive"
        export.Filter = "Zip Archive(*.zip)|*.zip|All Files(*.*)|*.*"
        If export.ShowDialog = True Then
            Dim file As New IO.FileInfo(My.Application.Info.DirectoryPath + "\" + SelectedFile.FileName), fs As IO.FileStream = IO.File.Open(file.FullName, IO.FileMode.OpenOrCreate)
            SelectedFile.Save(fs)
            fs.Close()
            Dim zip As New Ionic.Zip.ZipFile(export.FileName)
            zip.AddFile(file.FullName, "")
            zip.Save()
            My.Computer.FileSystem.DeleteFile(file.FullName)
        End If
    End Sub

#End Region

#Region "Export Image"

    Private Sub ExportImageMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles ExportImageMenuItem.Click
        Dim save As New Microsoft.Win32.SaveFileDialog
        save.Title = "Export Image"
        save.Filter = "Png Files(*.png)|*.png|All Files(*.*)|*.*"
        If save.ShowDialog Then
            Dim v As Visual = TryCast(SelectedFile, Visual),
            bmp As New RenderTargetBitmap(Convert.ToInt32(SelectedFile.ActualWidth), Convert.ToInt32(SelectedFile.ActualHeight), 120, 96, PixelFormats.Pbgra32)
            bmp.Render(v)
            Using fileStream = New IO.FileStream(save.FileName, IO.FileMode.OpenOrCreate, IO.FileAccess.Write)
                Dim encoder As BitmapEncoder = New PngBitmapEncoder()
                encoder.Frames.Add(BitmapFrame.Create(bmp))
                encoder.Save(fileStream)
            End Using
        End If
    End Sub

#End Region

#Region "FlowDocument"

    Public Shared Function ConvertTextDocumentToBlock(ByVal document As TextDocument, ByVal highlighter As IHighlighter) As Block
        If document Is Nothing Then
            Throw New ArgumentNullException("document")
        End If
        Dim p As New Paragraph()
        For Each line As DocumentLine In document.Lines
            Dim lineNumber As Integer = line.LineNumber
            Dim inlineBuilder As New HighlightedInlineBuilder(document.GetText(line))
            If highlighter IsNot Nothing Then
                Dim highlightedLine As HighlightedLine = highlighter.HighlightLine(lineNumber)
                Dim lineStartOffset As Integer = line.Offset
                For Each section As HighlightedSection In highlightedLine.Sections
                    inlineBuilder.SetHighlighting(section.Offset - lineStartOffset, section.Length, section.Color)
                Next
            End If
            p.Inlines.AddRange(inlineBuilder.CreateRuns())
            p.Inlines.Add(New LineBreak())
        Next
        Return p
    End Function

    Public Shared Function CreateFlowDocument(ByVal editor As ICSharpCode.AvalonEdit.TextEditor) As FlowDocument
        Dim highlighter As IHighlighter = TryCast(editor.TextArea.GetService(GetType(IHighlighter)), IHighlighter)
        Dim doc As New FlowDocument(ConvertTextDocumentToBlock(editor.Document, highlighter))
        doc.FontFamily = editor.FontFamily
        doc.FontSize = editor.FontSize
        Return doc
    End Function

    Private Sub FlowDocumentMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles FlowDocumentMenuItem.Click
        Dim export As New Microsoft.Win32.SaveFileDialog
        export.Title = "Export FlowDocument"
        export.Filter = "FlowDocument(*.xaml)|*.xaml|All Files(*.*)|*.*"
        If export.ShowDialog Then
            Dim file As New IO.FileInfo(export.FileName), fs As IO.FileStream = IO.File.Open(export.FileName, IO.FileMode.OpenOrCreate)
            Dim flow As FlowDocument = CreateFlowDocument(SelectedFile)
            Markup.XamlWriter.Save(flow, fs)
            fs.Close()
        End If
    End Sub

#End Region

#Region "Rich Text"

    Private Sub RichTextMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RichTextMenuItem.Click
        Dim export As New Microsoft.Win32.SaveFileDialog
        export.Title = "Export Rich Text"
        export.Filter = "Rich Text(*.rtf)|*.rtf|All Files(*.*)|*.*"
        If export.ShowDialog Then
            Dim file As New IO.FileInfo(export.FileName), fs As IO.FileStream = IO.File.Open(export.FileName, IO.FileMode.OpenOrCreate)
            Dim flow As FlowDocument = CreateFlowDocument(SelectedFile)
            Dim tr As New TextRange(flow.ContentStart, flow.ContentEnd)
            tr.Save(fs, Windows.DataFormats.Rtf)
            fs.Close()
        End If
    End Sub

#End Region

#Region "Webpage"

    Private Sub WebpageMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles WebpageMenuItem.Click
        Dim export As New Microsoft.Win32.SaveFileDialog
        export.Title = "Export Webpage"
        export.Filter = "Webpage(*.htm;*.html)|*.htm;*.html|All Files(*.*)|*.*"
        If export.ShowDialog Then
            Dim document As New TextDocument(SelectedFile.Text)
            Dim highlightDefinition As IHighlightingDefinition = SelectedFile.SyntaxHighlighting
            Dim highlighter As IHighlighter = New DocumentHighlighter(document, highlightDefinition.MainRuleSet)
            Dim html As String = HtmlClipboard.CreateHtmlFragment(document, highlighter, Nothing, New HtmlOptions)
            Dim writer As New IO.StreamWriter(export.FileName)
            writer.Write(html)
            writer.Close()
        End If
    End Sub

#End Region

#End Region

#Region "Print"

    Private Sub PageSetupMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles PageSetupMenuItem.Click
        Dim p As New PrintDialog
        If p.ShowDialog Then
            p.PrintVisual(SelectedFile, "test")
        End If
    End Sub

    Private Sub PrintMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles PrintMenuItem.Click, PrintButton.Click
        Dim p As New PrintDialog
        Try
            p.PrintVisual(SelectedFile, "test")
        Catch ex As Exception
            Dim m As New MessageBoxDialog("No printers found!", "Error", Nothing, Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/error32.png"))
            m.Owner = Me
            m.ShowDialog()
        End Try
    End Sub

    Private Sub PrintPreviewMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles PrintPreviewMenuItem.Click
        Dim preview As New PrintPreviewDialog
        preview.Owner = Me
        Dim s As String = System.IO.Path.GetRandomFileName
        Dim doc As New Xps.Packaging.XpsDocument(s, IO.FileAccess.ReadWrite)
        Dim xw As Xps.XpsDocumentWriter = Xps.Packaging.XpsDocument.CreateXpsDocumentWriter(doc)
        xw.Write(SelectedFile)
        doc.Close()
        Dim xpsdoc As New Xps.Packaging.XpsDocument(s, IO.FileAccess.Read)
        Dim fds As FixedDocumentSequence = xpsdoc.GetFixedDocumentSequence
        preview.DocumentViewer1.Document = fds
        Dim cc As ContentControl = TryCast(preview.DocumentViewer1.Template.FindName("PART_FindToolBarHost", preview.DocumentViewer1), ContentControl)
        cc.Visibility = Visibility.Collapsed
        preview.ShowDialog()
    End Sub

#End Region

#Region "Properties"

    Private Sub ReadOnlyMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles ReadOnlyMenuItem.Click
        Dim file As New IO.FileInfo(SelectedFile.FileFullName)
        If file.IsReadOnly Then
            file.IsReadOnly = False
            SelectedFile.IsReadOnly = False
            ReadOnlyMenuItem.IsChecked = False
        Else
            file.IsReadOnly = True
            SelectedFile.IsReadOnly = True
            ReadOnlyMenuItem.IsChecked = True
        End If
    End Sub

#End Region

#Region "Exit"

    Private Sub ExitMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles ExitMenuItem.Click
        Close()
    End Sub

#End Region

#End Region

#Region "Edit"

#Region "Undo/Redo"

    Private Sub UndoMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles UndoMenuItem.Click, UndoButton.Click
        SelectedFile.Undo()
    End Sub

    Private Sub RedoMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RedoMenuItem.Click, RedoButton.Click
        SelectedFile.Redo()
    End Sub

#End Region

#Region "Cut/Copy/Paste/Delete/Select All"

    Private Sub CutMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CutMenuItem.Click, CutButton.Click
        SelectedFile.Cut()
    End Sub

    Private Sub CopyMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CopyMenuItem.Click, CopyButton.Click
        SelectedFile.Copy()
    End Sub

    Private Sub PasteMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles PasteMenuItem.Click, PasteButton.Click
        SelectedFile.Paste()
    End Sub

    Private Sub DeleteMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles DeleteMenuItem.Click
        SelectedFile.SelectedText = ""
    End Sub

    Private Sub DeleteLineMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles DeleteLineMenuItem.Click
        ICSharpCode.AvalonEdit.AvalonEditCommands.DeleteLine.Execute(Nothing, SelectedFile.TextArea)
    End Sub

    Private Sub SelectAllMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles SelectAllMenuItem.Click
        SelectedFile.SelectAll()
    End Sub

#End Region

#Region "Find/Replace/Go To"

    Private Sub FindMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles FindMenuItem.Click, FindButton.Click
        Dim finddialog As New FindDialog
        finddialog.Owner = Me
        finddialog.ShowDialog()
        If finddialog.Res = "OK" Then
            SelectedFile.FindText(finddialog.TextBox1.Text)
        End If
    End Sub

    Private Sub ReplaceMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles ReplaceMenuItem.Click, ReplaceButton.Click
        Dim replace As New ReplaceDialog
        replace.Owner = Me
        replace.ShowDialog()
        If replace.Res = "OK" Then
            If replace.ReplaceAllCheckBox.IsChecked Then
                SelectedFile.ReplaceText(replace.TextBox1.Text, replace.TextBox2.Text, True)
            Else
                SelectedFile.ReplaceText(replace.TextBox1.Text, replace.TextBox2.Text, False)
            End If
        End If
    End Sub

    Private Sub GoToMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles GoToMenuItem.Click, GoToButton.Click
        Dim gotodialog As New GoToDialog
        gotodialog.Owner = Me
        gotodialog.ShowDialog()
        If gotodialog.Res = "OK" AndAlso gotodialog.Number <= SelectedFile.LineCount Then
            If gotodialog.Number <= SelectedFile.LineCount Then
                SelectedFile.Select(SelectedFile.Document.GetLineByNumber(gotodialog.Number).Offset, SelectedFile.Document.GetLineByNumber(gotodialog.Number).Length)
                SelectedFile.ScrollToLine(gotodialog.Number)
            Else
                Dim m As New MessageBoxDialog("Line not found!", "Error", Nothing, Nothing)
                m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/error32.png"))
                m.Owner = Me
                m.ShowDialog()
            End If
        End If
    End Sub

#End Region

#Region "Set Case"

    Private Sub InvertcaseMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles InvertcaseMenuItem.Click
        ICSharpCode.AvalonEdit.AvalonEditCommands.InvertCase.Execute(Nothing, SelectedFile.TextArea.TextView)
    End Sub

    Private Sub TitlecaseMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles TitlecaseMenuItem.Click
        ICSharpCode.AvalonEdit.AvalonEditCommands.ConvertToTitleCase.Execute(Nothing, SelectedFile.TextArea.TextView)
    End Sub

    Private Sub UppercaseMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles UppercaseMenuItem.Click
        ICSharpCode.AvalonEdit.AvalonEditCommands.ConvertToUppercase.Execute(Nothing, SelectedFile.TextArea.TextView)
    End Sub

    Private Sub LowercaseMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles LowercaseMenuItem.Click
        ICSharpCode.AvalonEdit.AvalonEditCommands.ConvertToLowercase.Execute(Nothing, SelectedFile.TextArea.TextView)
    End Sub

#End Region

#Region "Indent"

    Private Sub IndentMoreMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles IndentMoreMenuItem.Click
        SelectedFile.Document.Insert(SelectedFile.CaretOffset, vbTab)
    End Sub

    Private Sub IndentLessMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles IndentLessMenuItem.Click
        Try
            If SelectedFile.Document.GetText(SelectedFile.CaretOffset - 1, 1) = vbTab Then
                SelectedFile.Document.Remove(SelectedFile.CaretOffset - 1, 1)
            End If
        Catch ex As Exception
        End Try
    End Sub

#End Region

#End Region

#Region "View"

#Region "Menubar"

    Private Sub MenubarMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles MenubarMenuItem.Click
        If MainMenu.Visibility = Windows.Visibility.Collapsed Then
            MainMenu.Visibility = Windows.Visibility.Visible
            MenubarMenuItem.IsChecked = True
            UpdateUILayout()
        Else
            MainMenu.Visibility = Windows.Visibility.Collapsed
            MenubarMenuItem.IsChecked = False
            UpdateUILayout()
        End If
    End Sub

#End Region

#Region "Toolbar"

    Private Sub ToolbarMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles ToolbarMenuItem.Click
        If ToolBar1.Visibility = Windows.Visibility.Collapsed Then
            ToolBar1.Visibility = Windows.Visibility.Visible
            ToolbarMenuItem.IsChecked = True
            UpdateUILayout()
        Else
            ToolBar1.Visibility = Windows.Visibility.Collapsed
            ToolbarMenuItem.IsChecked = False
            UpdateUILayout()
        End If
    End Sub

#End Region

#Region "Statusbar"

    Private Sub StatusMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles StatusMenuItem.Click
        If StatusBar1.Visibility = Windows.Visibility.Visible Then
            StatusBar1.Visibility = Windows.Visibility.Collapsed
            StatusMenuItem.IsChecked = False
            UpdateUILayout()
        Else
            StatusBar1.Visibility = Windows.Visibility.Visible
            StatusMenuItem.IsChecked = True
            UpdateUILayout()
        End If
    End Sub

#End Region

#Region "Zoom"

    Private Sub ZoomInMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles ZoomInMenuItem.Click, ZoomInButton.Click
        SelectedFile.ZoomLevel += 0.1
        Dim t As New ScaleTransform(SelectedFile.ZoomLevel, SelectedFile.ZoomLevel)
        SelectedFile.TextArea.LayoutTransform = t
        UpdateButtons()
    End Sub

    Private Sub ZoomOutMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles ZoomOutMenuItem.Click, ZoomOutButton.Click
        SelectedFile.ZoomLevel += -0.1
        Dim t As New ScaleTransform(SelectedFile.ZoomLevel, SelectedFile.ZoomLevel)
        SelectedFile.TextArea.LayoutTransform = t
        UpdateButtons()
    End Sub

    Private Sub ZoomResetMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles ZoomResetMenuItem.Click, ResetZoomButton.Click
        SelectedFile.ZoomLevel = 1
        Dim t As New ScaleTransform(SelectedFile.ZoomLevel, SelectedFile.ZoomLevel)
        SelectedFile.TextArea.LayoutTransform = t
        UpdateButtons()
    End Sub

#End Region

#Region "Always On Top"

    Private Sub AlwaysOnTopMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles AlwaysOnTopMenuItem.Click
        If AlwaysOnTopMenuItem.IsChecked Then
            Topmost = True
        Else
            Topmost = False
        End If
    End Sub

#End Region

#Region "FullScreen"

    Private fullscreensetting As WindowState = Windows.WindowState.Normal
    Private Sub FullScreenMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles FullScreenMenuItem.Click
        If FullScreenMenuItem.IsChecked Then
            FullScreenMenuItem.IsChecked = False
            AlwaysOnTopMenuItem.IsEnabled = True
            If AlwaysOnTopMenuItem.IsChecked Then
                Topmost = True
            Else
                Topmost = False
            End If
            WindowStyle = Windows.WindowStyle.SingleBorderWindow
            WindowState = fullscreensetting
        Else
            AlwaysOnTopMenuItem.IsEnabled = False
            FullScreenMenuItem.IsChecked = True
            fullscreensetting = WindowState
            If WindowState = Windows.WindowState.Maximized Then
                WindowState = Windows.WindowState.Normal
            End If
            WindowStyle = Windows.WindowStyle.None
            Topmost = True
            WindowState = Windows.WindowState.Maximized
        End If
    End Sub

#End Region

#End Region

#Region "Insert"

    Private Sub InsertCommentMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles InsertCommentMenuItem.Click
        Dim f As New IO.FileInfo(SelectedFile.FileName)
        If f.Extension.ToLower = ".asm" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, ";Comment")
        ElseIf f.Extension.ToLower = ".boo" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "#Comment")
        ElseIf f.Extension.ToLower = ".cpp" OrElse f.Extension.ToLower = ".h" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "//Comment")
        ElseIf f.Extension.ToLower = ".cs" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "//Comment")
        ElseIf f.Extension.ToLower = ".css" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "/*Comment*/")
        ElseIf f.Extension.ToLower = ".gml" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "//Comment")
        ElseIf f.Extension.ToLower = ".html" OrElse f.Extension.ToLower = ".htm" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "<!--Comment-->")
        ElseIf f.Extension.ToLower = ".ini" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, ";Comment")
        ElseIf f.Extension.ToLower = ".lua" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "--Comment")
        ElseIf f.Extension.ToLower = ".php" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "//Comment")
        ElseIf f.Extension.ToLower = ".vb" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "'Comment")
        ElseIf f.Extension.ToLower = ".xml" Then
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, "<!--Comment-->")
        End If
    End Sub

    Private Sub TextFileMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles TextFileMenuItem.Click
        Dim open As New Microsoft.Win32.OpenFileDialog
        open.Title = "Insert Text File"
        open.Filter = "Text Files(*.txt)|*.txt|All Files(*.*)|*.*"
        If open.ShowDialog Then
            Dim s As String = My.Computer.FileSystem.ReadAllText(open.FileName)
            SelectedFile.Document.Insert(SelectedFile.CaretOffset, s)
        End If
    End Sub

    Private Sub DateMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles DateMenuItem.Click
        SelectedFile.SelectedText = Date.Now.ToString("M/dd/yyyy")
    End Sub

    Private Sub TimeMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles TimeMenuItem.Click
        SelectedFile.SelectedText = Date.Now.ToString("h:mm tt")
    End Sub

#End Region

#Region "Encoding"

    Private Sub DefaultEncodingMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles DefaultEncodingMenuItem.Click
        SelectedFile.Encoding = Text.Encoding.Default
        UpdateButtons()
    End Sub

    Private Sub ANSIMenuitem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles ANSIMenuitem.Click
        SelectedFile.Encoding = Text.Encoding.ASCII
        UpdateButtons()
    End Sub

    Private Sub UTF7MenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles UTF7MenuItem.Click
        SelectedFile.Encoding = Text.Encoding.UTF7
        UpdateButtons()
    End Sub

    Private Sub UTF8MenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles UTF8MenuItem.Click
        SelectedFile.Encoding = Text.Encoding.UTF8
        UpdateButtons()
    End Sub

    Private Sub UTF16MenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles UTF16MenuItem.Click
        SelectedFile.Encoding = Text.Encoding.BigEndianUnicode
        UpdateButtons()
    End Sub

    Private Sub UTF32MenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles UTF32MenuItem.Click
        SelectedFile.Encoding = Text.Encoding.UTF32
        UpdateButtons()
    End Sub

#End Region

#Region "Syntax"

    Private Sub DisableSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles DisableSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("Text")
        ClearSyntax()
        DisableSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub ASMSyntaxMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles ASMSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("ASM")
        ClearSyntax()
        ASMSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub ASPSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles ASPSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("ASP/XHTML")
        ClearSyntax()
        ASPSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub BooSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles BooSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("Boo")
        ClearSyntax()
        BooSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub CSSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CSSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("C#")
        ClearSyntax()
        CSSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub CSSSyntaxMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles CSSSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("CSS")
        ClearSyntax()
        CSSSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub CPPSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles CPPSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("C++")
        ClearSyntax()
        CPPSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub GMLSyntaxMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles GMLSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("GML")
        ClearSyntax()
        GMLSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub HTMLSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles HTMLSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("HTML")
        ClearSyntax()
        HTMLSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub INISyntaxMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles INISyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("INI")
        ClearSyntax()
        INISyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub JavaSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles JavaSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("Java")
        ClearSyntax()
        JavaSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub JavaScriptSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles JavaScriptSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("JavaScript")
        ClearSyntax()
        JavaScriptSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub LUASyntaxMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles LUASyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("LUA")
        ClearSyntax()
        LUASyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub PerlSyntaxMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles PerlSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("PERL")
        ClearSyntax()
        PerlSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub PHPSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles PHPSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("PHP")
        ClearSyntax()
        PHPSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub SQLSyntaxMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles SQLSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("SQL")
        ClearSyntax()
        SQLSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub VBSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles VBSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("VBNET")
        ClearSyntax()
        VBSyntaxMenuItem.IsChecked = True
    End Sub

    Private Sub XMLSyntaxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles XMLSyntaxMenuItem.Click
        SelectedFile.SyntaxHighlighting = ICSharpCode.AvalonEdit.Highlighting.HighlightingManager.Instance.GetDefinition("XML")
        ClearSyntax()
        XMLSyntaxMenuItem.IsChecked = True
    End Sub

#End Region

#Region "Run"

    Private Sub RunFileMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RunFileMenuItem.Click
        Try
            If SelectedFile.FileFullName IsNot Nothing Then
                Process.Start(SelectedFile.FileFullName)
            Else
                Dim fs As IO.FileStream = IO.File.Open(My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName, IO.FileMode.Create)
                SelectedFile.Save(fs)
                fs.Close()
                Dim s As String = My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName
                Process.Start(s)
            End If
        Catch ex As Exception
            Dim m As New MessageBoxDialog("File not found!", "Error", Nothing, Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/error32.png"))
            m.Owner = Me
            m.ShowDialog()
        End Try
    End Sub

    Private Sub RunInChromeMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RunInChromeMenuItem.Click
        Try
            If SelectedFile.FileFullName IsNot Nothing Then
                Process.Start(My.Settings.Options_Browser_Chrome, "file:///" + SelectedFile.FileFullName.Replace(" ", "%20"))
            Else
                Dim fs As IO.FileStream = IO.File.Open(My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName, IO.FileMode.Create)
                SelectedFile.Save(fs)
                fs.Close()
                Dim s As String = My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName
                Process.Start(My.Settings.Options_Browser_Chrome, "file:///" + s.Replace(" ", "%20"))
            End If
        Catch ex As Exception
            Dim m As New MessageBoxDialog("Chrome not found! Do you want to find it?", "Run", Nothing, Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
            m.Owner = Me
            m.ShowDialog()
            If m.Result = "Yes" Then
                Dim open As New Microsoft.Win32.OpenFileDialog
                open.Filter = "Chrome(.exe)|*.exe"
                open.FileName = "chrome.exe"
                If open.ShowDialog = True Then
                    My.Settings.Options_Browser_Chrome = open.FileName
                End If
            End If
        End Try
    End Sub

    Private Sub RunInFirefoxMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RunInFirefoxMenuItem.Click
        Try
            If SelectedFile.FileFullName IsNot Nothing Then
                Process.Start(My.Settings.Options_Browser_Firefox, "file:///" + SelectedFile.FileFullName.Replace(" ", "%20"))
            Else
                Dim fs As IO.FileStream = IO.File.Open(My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName, IO.FileMode.Create)
                SelectedFile.Save(fs)
                fs.Close()
                Dim s As String = My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName
                Process.Start(My.Settings.Options_Browser_Firefox, "file:///" + s.Replace(" ", "%20"))
            End If
        Catch ex As Exception
            Dim m As New MessageBoxDialog("Firefox not found! Do you want to find it?", "Run", Nothing, Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
            m.Owner = Me
            m.ShowDialog()
            If m.Result = "Yes" Then
                Dim open As New Microsoft.Win32.OpenFileDialog
                open.Filter = "Firefox(.exe)|*.exe"
                open.FileName = "firefox.exe"
                If open.ShowDialog = True Then
                    My.Settings.Options_Browser_Firefox = open.FileName
                End If
            End If
        End Try
    End Sub

    Private Sub RunInIEMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RunInIEMenuItem.Click
        Try
            If SelectedFile.FileFullName IsNot Nothing Then
                Process.Start(My.Settings.Options_Browser_IE, "file:///" + SelectedFile.FileFullName.Replace(" ", "%20"))
            Else
                Dim fs As IO.FileStream = IO.File.Open(My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName, IO.FileMode.Create)
                SelectedFile.Save(fs)
                fs.Close()
                Dim s As String = My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName
                Process.Start(My.Settings.Options_Browser_IE, "file:///" + s.Replace(" ", "%20"))
            End If
        Catch ex As Exception
            Dim m As New MessageBoxDialog("IE not found! Do you want to find it?", "Run", Nothing, Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
            m.Owner = Me
            m.ShowDialog()
            If m.Result = "Yes" Then
                Dim open As New Microsoft.Win32.OpenFileDialog
                open.Filter = "iexplore(.exe)|*.exe"
                open.FileName = "iexplore.exe"
                If open.ShowDialog = True Then
                    My.Settings.Options_Browser_IE = open.FileName
                End If
            End If
        End Try
    End Sub

    Private Sub RunInOperaMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RunInOperaMenuItem.Click
        Try
            If SelectedFile.FileFullName IsNot Nothing Then
                Process.Start(My.Settings.Options_Browser_Opera, "file:///" + SelectedFile.FileFullName.Replace(" ", "%20"))
            Else
                Dim fs As IO.FileStream = IO.File.Open(My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName, IO.FileMode.Create)
                SelectedFile.Save(fs)
                fs.Close()
                Dim s As String = My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName
                Process.Start(My.Settings.Options_Browser_Opera, "file:///" + s.Replace(" ", "%20"))
            End If
        Catch ex As Exception
            Dim m As New MessageBoxDialog("Opera not found! Do you want to find it?", "Run", Nothing, Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
            m.Owner = Me
            m.ShowDialog()
            If m.Result = "Yes" Then
                Dim open As New Microsoft.Win32.OpenFileDialog
                open.Filter = "Opera(.exe)|*.exe"
                open.FileName = "opera.exe"
                If open.ShowDialog = True Then
                    My.Settings.Options_Browser_Opera = open.FileName
                End If
            End If
        End Try
    End Sub

    Private Sub RunInSafariMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles RunInSafariMenuItem.Click
        Try
            If SelectedFile.FileFullName IsNot Nothing Then
                Process.Start(My.Settings.Options_Browser_Safari, "file:///" + SelectedFile.FileFullName.Replace(" ", "%20"))
            Else
                Dim fs As IO.FileStream = IO.File.Open(My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName, IO.FileMode.Create)
                SelectedFile.Save(fs)
                fs.Close()
                Dim s As String = My.Computer.FileSystem.SpecialDirectories.Temp + "\" + SelectedFile.FileName
                Process.Start(My.Settings.Options_Browser_Safari, "file:///" + s.Replace(" ", "%20"))
            End If
        Catch ex As Exception
            Dim m As New MessageBoxDialog("Safari not found! Do you want to find it?", "Run", Nothing, Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
            m.Owner = Me
            m.ShowDialog()
            If m.Result = "Yes" Then
                Dim open As New Microsoft.Win32.OpenFileDialog
                open.Filter = "Safari(.exe)|*.exe"
                open.FileName = "safari.exe"
                If open.ShowDialog = True Then
                    My.Settings.Options_Browser_Safari = open.FileName
                End If
            End If
        End Try
    End Sub

#End Region

#Region "Navigation"

    Private Sub StartMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles StartMenuItem.Click
        SelectedFile.ScrollToHome()
    End Sub

    Private Sub EndMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles EndMenuItem.Click
        SelectedFile.ScrollToEnd()
    End Sub

    Private Sub LineDownMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles LineDownMenuItem.Click
        SelectedFile.LineDown()
    End Sub

    Private Sub LineUpMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles LineUpMenuItem.Click
        SelectedFile.LineUp()
    End Sub

    Private Sub LineLeftMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles LineLeftMenuItem.Click
        SelectedFile.LineLeft()
    End Sub

    Private Sub LineRightMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles LineRightMenuItem.Click
        SelectedFile.LineRight()
    End Sub

    Private Sub PageDownMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles PageDownMenuItem.Click
        SelectedFile.PageDown()
    End Sub

    Private Sub PageUpMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles PageUpMenuItem.Click
        SelectedFile.PageUp()
    End Sub

    Private Sub PageLeftMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles PageLeftMenuItem.Click
        SelectedFile.PageLeft()
    End Sub

    Private Sub PageRightMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles PageRightMenuItem.Click
        SelectedFile.PageRight()
    End Sub

    Private Sub ScrollToCaretMenuItem_Click(sender As Object, e As System.Windows.RoutedEventArgs) Handles ScrollToCaretMenuItem.Click
        SelectedFile.ScrollTo(SelectedFile.caret.Line, SelectedFile.caret.Column)
    End Sub

#End Region

#Region "Tools"

    Private Sub RunExTool(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs)
        Dim i As MenuItem = TryCast(e.Source, MenuItem)
        Try
            Process.Start(TryCast(i.ToolTip, String))
        Catch ex As Exception
            Dim m As New MessageBoxDialog("Program not found!", "Error", Nothing, Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/error32.png"))
            m.Owner = Me
            m.ShowDialog()
        End Try
    End Sub

    Private Sub ExternalToolsMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles ExternalToolsMenuItem.Click
        Dim extools As New ExternalToolsDialog
        extools.Owner = Me
        extools.ShowDialog()
        If extools.Res = "OK" Then
            My.Settings.ExTools.Clear()
            For Each s As String In extools.ListBox1.Items
                My.Settings.ExTools.Add(s)
            Next
        End If
    End Sub

    Private Sub OptionsMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles OptionsMenuItem.Click
        Dim d As New OptionsDialog
        d.Owner = Me
        d.ShowDialog()
        If d.Res = "OK" Then
            If d.StartupComboBox.SelectedIndex = 0 Then
                For Each file As FileTab In TabCell.Children
                    file.EditorHeader.CloseButton.Visibility = Visibility.Visible
                Next
            ElseIf d.StartupComboBox.SelectedIndex = 1 Then
                For Each file As FileTab In TabCell.Children
                    If file.IsSelected Then
                        file.EditorHeader.CloseButton.Visibility = Visibility.Visible
                    Else
                        file.EditorHeader.CloseButton.Visibility = Visibility.Collapsed
                    End If
                Next
            End If
            If d.ShowLineNumbersCheckBox.IsChecked Then
                For Each file As FileTab In TabCell.Children
                    file.Editor.ShowLineNumbers = True
                Next
            Else
                For Each file As FileTab In TabCell.Children
                    file.Editor.ShowLineNumbers = False
                Next
            End If
        End If
    End Sub

#End Region

#Region "Help"

#Region "Online Help"

    Private Sub OnlineHelpMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles OnlineHelpMenuItem.Click
        Process.Start("http://devpad.codeplex.com/documentation")
    End Sub

#End Region

#Region "Website"

    Private Sub WebsiteMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles WebsiteMenuItem.Click
        Process.Start("http://semagsoft.com")
    End Sub

#End Region

#Region "Donate"

    Private Sub DonateMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles DonateMenuItem.Click
        Process.Start("https://www.paypal.com/us/cgi-bin/webscr?cmd=_flow&SESSION=EhW2MocbvjnBxdX-wbJbVkNK4Djd3ZkRVeFbYF8A1-YKC6cMSi79BUYe8ey&dispatch=5885d80a13c0db1f8e263663d3faee8deaa77efc63a6eb429928d42bdf5d9d2c")
    End Sub

#End Region

#Region "Check For Update"

    Private Sub CheckForUpdatesMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles CheckForUpdatesMenuItem.Click
        Dim update As New UpdateDialog
        update.Owner = Me
        update.ShowDialog()
    End Sub

#End Region

#Region "About"

    Private Sub AboutMenuItem_Click(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs) Handles AboutMenuItem.Click
        Dim a As New AboutDialog
        a.Owner = Me
        a.ShowDialog()
    End Sub

#End Region

#End Region

#End Region

#Region "TabCell"

#Region "SelectionChanged"

    'Private Sub TabCell_SelectionChanged(ByVal sender As Object, ByVal e As System.Windows.Controls.SelectionChangedEventArgs) 'TODO: Handles TabCell.SelectionChanged
    '    If TabCell.SelectedContent IsNot Nothing Then
    '        SelectedFile = TryCast(TabCell.SelectedContent, FileTab)
    '        Dim f As New IO.FileInfo(SelectedFile.FileName)
    '        UpdateSyntax(f.Extension)
    '    Else
    '        SelectedFile = Nothing
    '    End If
    '    UpdateButtons()
    '    If My.Settings.Options_CloseButtonMade = 1 Then
    '        For Each file As FileTab In TabCell.Children
    '            If file.IsSelected Then
    '                file.EditorHeader.CloseButton.Visibility = Visibility.Visible
    '            Else
    '                file.EditorHeader.CloseButton.Visibility = Visibility.Collapsed
    '            End If
    '        Next
    '    End If
    'End Sub

#End Region

#Region "CloseFile"

    Private Sub CloseFile(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs)
        Dim header As TabHeader = TryCast(e.Source, TabHeader), file As FileTab = TryCast(header.Parent, FileTab)
        If file.Editor.FileChanged Then
            Dim m As New MessageBoxDialog("Do you want to save " + file.Editor.FileName + "?", "Save", "YesNoCancel", Nothing)
            m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
            m.Owner = Me
            m.ShowDialog()
            If m.Result = "Yes" Then
                If file.Editor.FileFullName IsNot Nothing Then
                    file.Editor.Save(file.Editor.FileFullName)
                    file.Close()
                Else
                    Dim Savedialog As New Microsoft.Win32.SaveFileDialog
                    Savedialog.AddExtension = True
                    Savedialog.Filter = SupportedFiles
                    Savedialog.FileName = file.Editor.FileName
                    If Savedialog.ShowDialog Then
                        file.Editor.Save(Savedialog.FileName)
                        file.Close()
                    End If
                End If
            ElseIf m.Result = "No" Then
                file.Close()
            End If
        Else
            file.Close()
        End If
    End Sub

#End Region

#Region "CloseAllButThis"

    Private Sub CloseAllButThis(ByVal sender As Object, ByVal e As System.Windows.RoutedEventArgs)
        Dim header As TabHeader = TryCast(e.Source, TabHeader), filetab As FileTab = TryCast(header.Parent, FileTab)
        While TabCell.ChildrenCount > 1
            If TabCell.Children.Item(0) IsNot filetab Then
                Dim t As FileTab = TryCast(TabCell.Children.Item(0), FileTab)
                If t.Editor.FileChanged Then
                    Dim m As New MessageBoxDialog("Do you want to save " + t.Editor.FileName + "?", "Save", "YesNoCancel", Nothing)
                    m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
                    m.Owner = Me
                    m.ShowDialog()
                    If m.Result = "Yes" Then
                        If t.Editor.FileFullName IsNot Nothing Then
                            t.Editor.Save(t.Editor.FileFullName)
                            TabCell.Children.RemoveAt(0)
                        Else
                            Dim d As New Microsoft.Win32.SaveFileDialog
                            d.AddExtension = True
                            d.Filter = SupportedFiles
                            d.FileName = t.Editor.FileName
                            If d.ShowDialog Then
                                Dim file As New IO.FileInfo(d.FileName)
                                t.Editor.Save(d.FileName)
                                TabCell.Children.RemoveAt(0)
                            End If
                        End If
                    ElseIf m.Result = "No" Then
                        TabCell.Children.RemoveAt(0)
                    Else
                        Exit While
                    End If
                Else
                    TabCell.Children.RemoveAt(0)
                End If
            Else
                Dim t As FileTab = TryCast(TabCell.Children.Item(1), FileTab)
                If t.Editor.FileChanged Then
                    Dim m As New MessageBoxDialog("Do you want to save " + t.Editor.FileName + "?", "Save", "YesNoCancel", Nothing)
                    m.MessageImage.Source = New BitmapImage(New Uri("pack://application:,,,/Images/Common/question32.png"))
                    m.Owner = Me
                    m.ShowDialog()
                    If m.Result = "Yes" Then
                        If t.Editor.FileFullName IsNot Nothing Then
                            t.Editor.Save(t.Editor.FileFullName)
                            TabCell.Children.RemoveAt(1)
                        Else
                            Dim d As New Microsoft.Win32.SaveFileDialog
                            d.AddExtension = True
                            d.Filter = SupportedFiles
                            d.FileName = t.Editor.FileName
                            If d.ShowDialog Then
                                Dim file As New IO.FileInfo(d.FileName)
                                t.Editor.Save(d.FileName)
                                TabCell.Children.RemoveAt(1)
                            End If
                        End If
                    ElseIf m.Result = "No" Then
                        TabCell.Children.RemoveAt(1)
                    Else
                        Exit While
                    End If
                Else
                    TabCell.Children.RemoveAt(1)
                End If
            End If
        End While
    End Sub

#End Region

#End Region

#Region "TODO"

    Private Sub HiddenMenuItem_Click(sender As Object, e As RoutedEventArgs) Handles HiddenMenuItem.Click
        'TODO:
        Dim file As New IO.FileInfo(SelectedFile.FileFullName)
        If (file.Attributes And IO.FileAttributes.Hidden) = IO.FileAttributes.Hidden Then
            file.Attributes = IO.FileAttributes.Hidden
            HiddenMenuItem.IsEnabled = False
        Else
            file.Attributes = IO.FileAttributes.Hidden
            HiddenMenuItem.IsEnabled = True
        End If
    End Sub

#End Region

    Private Sub dockingManager_ActiveContentChanged(sender As Object, e As EventArgs) Handles dockingManager.ActiveContentChanged
        SelectedFile = dockingManager.ActiveContent
        UpdateButtons()
    End Sub
End Class