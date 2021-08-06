Public Class CodeEditor
    Inherits ICSharpCode.AvalonEdit.TextEditor

    Public FileFullName As String = Nothing, FileFormat As String = Nothing, FileChanged As Boolean = False, FileName As String = Nothing
    Public ZoomLevel As Double = 1, completionWindow As ICSharpCode.AvalonEdit.CodeCompletion.CompletionWindow

    Public Shared UpdateSelectedText As RoutedEvent = EventManager.RegisterRoutedEvent("UpdateSelectedText", RoutingStrategy.Tunnel, GetType(RoutedEventHandler), GetType(CodeEditor))

    Public WithEvents caret As ICSharpCode.AvalonEdit.Editing.Caret = Me.TextArea.Caret

    Public FoldingManager As ICSharpCode.AvalonEdit.Folding.FoldingManager = ICSharpCode.AvalonEdit.Folding.FoldingManager.Install(TextArea)
    Public FoldingStrategy As New ICSharpCode.AvalonEdit.Folding.XmlFoldingStrategy

    Private com As ICSharpCode.AvalonEdit.AvalonEditCommands

    Public Sub New()
        BorderThickness = New Thickness(0, 0, 0, 0)
        If My.Settings.Options_ShowLineNumbers Then
            ShowLineNumbers = True
        End If
        If My.Settings.Options_DefaultEncoding = 0 Then
            Encoding = System.Text.Encoding.Default
        ElseIf My.Settings.Options_DefaultEncoding = 1 Then
            Encoding = System.Text.Encoding.ASCII
        ElseIf My.Settings.Options_DefaultEncoding = 2 Then
            Encoding = System.Text.Encoding.UTF7
        ElseIf My.Settings.Options_DefaultEncoding = 3 Then
            Encoding = System.Text.Encoding.UTF8
        ElseIf My.Settings.Options_DefaultEncoding = 4 Then
            Encoding = System.Text.Encoding.BigEndianUnicode
        ElseIf My.Settings.Options_DefaultEncoding = 5 Then
            Encoding = System.Text.Encoding.UTF32
        End If
        If My.Settings.Options_ScrollPastContent Then
            Me.Options.AllowScrollBelowDocument = True
        End If
        If My.Settings.Options_WordWrap Then
            Me.WordWrap = True
        End If
        FontFamily = My.Settings.Options_DefaultFont
        FontSize = My.Settings.Options_DefaultFontSize
        If My.Settings.Options_CodeCollapsing Then
            AddFoldingSupport()
        End If
    End Sub

    Public Sub IndentMore()
        ICSharpCode.AvalonEdit.AvalonEditCommands.IndentSelection.Execute(0, Me)
    End Sub

    'Private Sub textEditor_TextArea_TextEntered(ByVal sender As Object, ByVal e As TextCompositionEventArgs)
    '    If e.Text = "." Then
    '        ' Open code completion after the user has pressed dot:
    '        completionWindow = New ICSharpCode.AvalonEdit.CodeCompletion.CompletionWindow(TextArea)
    '        Dim data As IList(Of ICSharpCode.AvalonEdit.CodeCompletion.ICompletionData) = completionWindow.CompletionList.CompletionData
    '        data.Add(New MyCompletionData("Item1"))
    '        data.Add(New MyCompletionData("Item2"))
    '        data.Add(New MyCompletionData("Item3"))
    '        completionWindow.Show()
    '        'completionWindow.Closed += Function() Do
    '        completionWindow = Nothing
    '    End If
    'End Sub

    'Private Sub textEditor_TextArea_TextEntering(ByVal sender As Object, ByVal e As TextCompositionEventArgs)
    '    If e.Text.Length > 0 AndAlso completionWindow IsNot Nothing Then
    '        If Not Char.IsLetterOrDigit(e.Text(0)) Then
    '            ' Whenever a non-letter is typed while the completion window is open,
    '            ' insert the currently selected element.
    '            completionWindow.CompletionList.RequestInsertion(e)
    '        End If
    '    End If
    '    ' Do not set e.Handled=true.
    '    ' We still want to insert the character that was typed.
    'End Sub

    Private Sub CodeEditor_TextChanged(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.TextChanged
        'MessageBox.Show(Parent.ToString)
        If FileChanged = False Then
            FileChanged = True
        End If
        FoldingStrategy.UpdateFoldings(FoldingManager, Document)
    End Sub

#Region "Find\Replace"

    Public Sub FindText(ByVal s As String)
        Dim r As New Text.RegularExpressions.Regex(s)
        Dim m As Text.RegularExpressions.Match = r.Match(Text)
        If m.Success Then
            [Select](m.Index, m.Length)
        Else
            MessageBox.Show("Not Found")
        End If
    End Sub

    Public Sub ReplaceText(ByVal find As String, ByVal replace As String, ByVal replaceall As Boolean)
        If replaceall Then
            Dim r As New Text.RegularExpressions.Regex(find)
            Dim int As Integer = 0
            Try
                For Each m As Text.RegularExpressions.Match In r.Matches(Text)
                    If m.Success Then
                        Document.Replace(m.Index, m.Length, replace)
                        If int > 0 Then
                            Document.Remove(m.Index + m.Length + 1, int)
                        End If
                        If int = 0 Then
                            int = 1
                        End If
                    Else
                        Exit For
                    End If
                Next
            Catch ex As Exception
                MessageBox.Show(ex.Message)
            End Try
        Else
            Dim r As New Text.RegularExpressions.Regex(find)
            Dim m As Text.RegularExpressions.Match = r.Match(Text)
            If m.Success Then
                Document.Replace(m.Index, m.Length, replace)
            Else
                MessageBox.Show("Not Found")
            End If
        End If
    End Sub

#End Region

    Private Sub caret_PositionChanged(sender As Object, e As System.EventArgs) Handles caret.PositionChanged
        Me.RaiseEvent(New RoutedEventArgs(UpdateSelectedText))
    End Sub

    Public Sub AddFoldingSupport()
        If FileFormat = "XML" Then
            FoldingStrategy.UpdateFoldings(FoldingManager, Document)
        End If
    End Sub
End Class

'Public Class MyCompletionData
'    Implements ICSharpCode.AvalonEdit.CodeCompletion.ICompletionData
'    Public Sub New(ByVal text As String)
'        Me.Text = text
'    End Sub

'    Public ReadOnly Property Image() As System.Windows.Media.ImageSource
'        Get
'            Return Nothing
'        End Get
'    End Property

'    Public Property Text() As String
'        Get
'            Return m_Text
'        End Get
'        Private Set(ByVal value As String)
'            m_Text = Value
'        End Set
'    End Property
'    Private m_Text As String

'    ' Use this property if you want to show a fancy UIElement in the list.
'    Public ReadOnly Property Content() As Object
'        Get
'            Return Me.Text
'        End Get
'    End Property

'    Public ReadOnly Property Description() As Object
'        Get
'            Return "Description for " & Me.Text
'        End Get
'    End Property

'    Public Sub Complete(ByVal textArea As ICSharpCode.AvalonEdit.Editing.TextArea, ByVal completionSegment As ICSharpCode.AvalonEdit.Document.ISegment, ByVal insertionRequestEventArgs As EventArgs)
'        textArea.Document.Replace(completionSegment, Me.Text)
'    End Sub
'End Class