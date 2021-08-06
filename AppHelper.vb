Imports System.Net, System.Web
Public Class AppHelper
    Public Structure Margins
        Public Sub New(ByVal t As Thickness)
            Left = CInt(Fix(t.Left))
            Right = CInt(Fix(t.Right))
            Top = CInt(Fix(t.Top))
            Bottom = CInt(Fix(t.Bottom))
        End Sub
        Public Left As Integer, Right As Integer, Top As Integer, Bottom As Integer
    End Structure

    Public Structure blogInfo
        Public title As String
        Public description As String
    End Structure

    Public Interface IgetCatList
        <CookComputing.XmlRpc.XmlRpcMethod("metaWeblog.newPost")> _
        Function NewPage(ByVal blogId As Integer, ByVal strUserName As String, ByVal strPassword As String, ByVal content As AppHelper.blogInfo, ByVal publish As Integer) As String
    End Interface

    <Runtime.InteropServices.DllImport("dwmapi.dll", PreserveSig:=False)> _
    Shared Sub DwmExtendFrameIntoClientArea(ByVal hwnd As IntPtr, ByRef pMargins As Margins)
    End Sub

    <Runtime.InteropServices.DllImport("dwmapi.dll", PreserveSig:=False)> _
    Shared Function DwmIsCompositionEnabled() As Boolean
    End Function

    Public Shared ReadOnly Property IsGlassEnabled() As Boolean
        Get
            Return DwmIsCompositionEnabled()
        End Get
    End Property

    Public Shared Function ExtendGlassFrame(ByVal window As Window, ByVal margin As Thickness) As Boolean
        If Not DwmIsCompositionEnabled() Then
            Return False
        End If
        Dim hwnd As IntPtr = New Interop.WindowInteropHelper(window).Handle, margins As New Margins(margin),
            background As New SolidColorBrush(Colors.Red)
        If hwnd = IntPtr.Zero Then
            Throw New InvalidOperationException("The Window must be shown before extending glass.")
        End If
        background.Opacity = 0.5
        window.Background = Brushes.Transparent
        Interop.HwndSource.FromHwnd(hwnd).CompositionTarget.BackgroundColor = Colors.Transparent
        DwmExtendFrameIntoClientArea(hwnd, margins)
        Return True
    End Function

    Public Shared Sub DisableGlassFrame(ByVal window As Window)
        Dim hwnd As IntPtr = New Interop.WindowInteropHelper(window).Handle
        If hwnd = IntPtr.Zero Then
            Throw New InvalidOperationException("The Window must be shown before extending glass.")
        End If
        Interop.HwndSource.FromHwnd(hwnd).CompositionTarget.BackgroundColor = Colors.White
    End Sub
End Class