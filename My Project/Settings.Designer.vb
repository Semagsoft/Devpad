'------------------------------------------------------------------------------
' <auto-generated>
'     This code was generated by a tool.
'     Runtime Version:4.0.30319.42000
'
'     Changes to this file may cause incorrect behavior and will be lost if
'     the code is regenerated.
' </auto-generated>
'------------------------------------------------------------------------------

Option Strict On
Option Explicit On



<Global.System.Runtime.CompilerServices.CompilerGeneratedAttribute(),  _
 Global.System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.Editors.SettingsDesigner.SettingsSingleFileGenerator", "16.10.0.0"),  _
 Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Advanced)>  _
Partial Friend NotInheritable Class MySettings
    Inherits Global.System.Configuration.ApplicationSettingsBase
    
    Private Shared defaultInstance As MySettings = CType(Global.System.Configuration.ApplicationSettingsBase.Synchronized(New MySettings()),MySettings)
    
#Region "My.Settings Auto-Save Functionality"
#If _MyType = "WindowsForms" Then
    Private Shared addedHandler As Boolean

    Private Shared addedHandlerLockObject As New Object

    <Global.System.Diagnostics.DebuggerNonUserCodeAttribute(), Global.System.ComponentModel.EditorBrowsableAttribute(Global.System.ComponentModel.EditorBrowsableState.Advanced)> _
    Private Shared Sub AutoSaveSettings(sender As Global.System.Object, e As Global.System.EventArgs)
        If My.Application.SaveMySettingsOnExit Then
            My.Settings.Save()
        End If
    End Sub
#End If
#End Region
    
    Public Shared ReadOnly Property [Default]() As MySettings
        Get
            
#If _MyType = "WindowsForms" Then
               If Not addedHandler Then
                    SyncLock addedHandlerLockObject
                        If Not addedHandler Then
                            AddHandler My.Application.Shutdown, AddressOf AutoSaveSettings
                            addedHandler = True
                        End If
                    End SyncLock
                End If
#End If
            Return defaultInstance
        End Get
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("480, 420")>  _
    Public Property MainWindow_Size() As Global.System.Drawing.Size
        Get
            Return CType(Me("MainWindow_Size"),Global.System.Drawing.Size)
        End Get
        Set
            Me("MainWindow_Size") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("32, 32")>  _
    Public Property MainWindow_Loc() As Global.System.Drawing.Point
        Get
            Return CType(Me("MainWindow_Loc"),Global.System.Drawing.Point)
        End Get
        Set
            Me("MainWindow_Loc") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("False")>  _
    Public Property MainWindow_IsMax() As Boolean
        Get
            Return CType(Me("MainWindow_IsMax"),Boolean)
        End Get
        Set
            Me("MainWindow_IsMax") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("0")>  _
    Public Property Options_StartupMode() As Integer
        Get
            Return CType(Me("Options_StartupMode"),Integer)
        End Get
        Set
            Me("Options_StartupMode") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("True")>  _
    Public Property MainWindow_ShowStatusbar() As Boolean
        Get
            Return CType(Me("MainWindow_ShowStatusbar"),Boolean)
        End Get
        Set
            Me("MainWindow_ShowStatusbar") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("True")>  _
    Public Property MainWindow_ShowToolbar() As Boolean
        Get
            Return CType(Me("MainWindow_ShowToolbar"),Boolean)
        End Get
        Set
            Me("MainWindow_ShowToolbar") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("True")>  _
    Public Property Options_ShowLineNumbers() As Boolean
        Get
            Return CType(Me("Options_ShowLineNumbers"),Boolean)
        End Get
        Set
            Me("Options_ShowLineNumbers") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("0")>  _
    Public Property Options_CloseButtonMade() As Integer
        Get
            Return CType(Me("Options_CloseButtonMade"),Integer)
        End Get
        Set
            Me("Options_CloseButtonMade") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("True")>  _
    Public Property MainWindow_ShowMenubar() As Boolean
        Get
            Return CType(Me("MainWindow_ShowMenubar"),Boolean)
        End Get
        Set
            Me("MainWindow_ShowMenubar") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("<?xml version=""1.0"" encoding=""utf-16""?>"&Global.Microsoft.VisualBasic.ChrW(13)&Global.Microsoft.VisualBasic.ChrW(10)&"<ArrayOfString xmlns:xsi=""http://www.w3."& _ 
        "org/2001/XMLSchema-instance"" xmlns:xsd=""http://www.w3.org/2001/XMLSchema"" />")>  _
    Public Property ExTools() As Global.System.Collections.Specialized.StringCollection
        Get
            Return CType(Me("ExTools"),Global.System.Collections.Specialized.StringCollection)
        End Get
        Set
            Me("ExTools") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("<?xml version=""1.0"" encoding=""utf-16""?>"&Global.Microsoft.VisualBasic.ChrW(13)&Global.Microsoft.VisualBasic.ChrW(10)&"<ArrayOfString xmlns:xsi=""http://www.w3."& _ 
        "org/2001/XMLSchema-instance"" xmlns:xsd=""http://www.w3.org/2001/XMLSchema"" />")>  _
    Public Property Options_RecentFiles() As Global.System.Collections.Specialized.StringCollection
        Get
            Return CType(Me("Options_RecentFiles"),Global.System.Collections.Specialized.StringCollection)
        End Get
        Set
            Me("Options_RecentFiles") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("0")>  _
    Public Property Options_DefaultFormat() As Integer
        Get
            Return CType(Me("Options_DefaultFormat"),Integer)
        End Get
        Set
            Me("Options_DefaultFormat") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("0")>  _
    Public Property Options_DefaultEncoding() As Integer
        Get
            Return CType(Me("Options_DefaultEncoding"),Integer)
        End Get
        Set
            Me("Options_DefaultEncoding") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("Courier New")>  _
    Public Property Options_DefaultFont() As Global.System.Windows.Media.FontFamily
        Get
            Return CType(Me("Options_DefaultFont"),Global.System.Windows.Media.FontFamily)
        End Get
        Set
            Me("Options_DefaultFont") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("14")>  _
    Public Property Options_DefaultFontSize() As Double
        Get
            Return CType(Me("Options_DefaultFontSize"),Double)
        End Get
        Set
            Me("Options_DefaultFontSize") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("chrome.exe")>  _
    Public Property Options_Browser_Chrome() As String
        Get
            Return CType(Me("Options_Browser_Chrome"),String)
        End Get
        Set
            Me("Options_Browser_Chrome") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("True")>  _
    Public Property Options_CodeCollapsing() As Boolean
        Get
            Return CType(Me("Options_CodeCollapsing"),Boolean)
        End Get
        Set
            Me("Options_CodeCollapsing") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("firefox.exe")>  _
    Public Property Options_Browser_Firefox() As String
        Get
            Return CType(Me("Options_Browser_Firefox"),String)
        End Get
        Set
            Me("Options_Browser_Firefox") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("iexplore.exe")>  _
    Public Property Options_Browser_IE() As String
        Get
            Return CType(Me("Options_Browser_IE"),String)
        End Get
        Set
            Me("Options_Browser_IE") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("opera.exe")>  _
    Public Property Options_Browser_Opera() As String
        Get
            Return CType(Me("Options_Browser_Opera"),String)
        End Get
        Set
            Me("Options_Browser_Opera") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("safari.exe")>  _
    Public Property Options_Browser_Safari() As String
        Get
            Return CType(Me("Options_Browser_Safari"),String)
        End Get
        Set
            Me("Options_Browser_Safari") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("True")>  _
    Public Property Options_ScrollPastContent() As Boolean
        Get
            Return CType(Me("Options_ScrollPastContent"),Boolean)
        End Get
        Set
            Me("Options_ScrollPastContent") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("False")>  _
    Public Property Options_WordWrap() As Boolean
        Get
            Return CType(Me("Options_WordWrap"),Boolean)
        End Get
        Set
            Me("Options_WordWrap") = value
        End Set
    End Property
    
    <Global.System.Configuration.UserScopedSettingAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Configuration.DefaultSettingValueAttribute("2")>  _
    Public Property Options_Theme() As Integer
        Get
            Return CType(Me("Options_Theme"),Integer)
        End Get
        Set
            Me("Options_Theme") = value
        End Set
    End Property
End Class

Namespace My
    
    <Global.Microsoft.VisualBasic.HideModuleNameAttribute(),  _
     Global.System.Diagnostics.DebuggerNonUserCodeAttribute(),  _
     Global.System.Runtime.CompilerServices.CompilerGeneratedAttribute()>  _
    Friend Module MySettingsProperty
        
        <Global.System.ComponentModel.Design.HelpKeywordAttribute("My.Settings")>  _
        Friend ReadOnly Property Settings() As Global.Devpad.MySettings
            Get
                Return Global.Devpad.MySettings.Default
            End Get
        End Property
    End Module
End Namespace
