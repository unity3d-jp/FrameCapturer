using System;
using UnityEngine;
using UnityEngine.Recorder.FrameRecorder;

namespace UnityEditor.Recorder.FrameRecorder
{
    public abstract class DefaultImageRecorderSettingsEditor : RecorderSettingsEditor
    {
        string[] m_Displays;

        protected override void OnEnable()
        {
            base.OnEnable();
            var displayCount = Display.displays.Length;
            m_Displays = new string[displayCount];
            for (int i = 0; i < displayCount; i++)
                m_Displays[i] = string.Format("Display {0}", i + 1);
        }

        protected override void OnInputGui()
        {
            var settingsObj = serializedObject.targetObject as ImageRecorderSettings;

            m_LayoutHelper.AddEnumProperty("Source:", serializedObject, () => settingsObj.m_InputType);

            switch (settingsObj.m_InputType)
            {
                case EImageSourceType.GameDisplay:
                {
                    m_LayoutHelper.indentLevel++;
                    m_LayoutHelper.AddEnumProperty("Display:", serializedObject, () => settingsObj.m_ScreenID, m_Displays);
                    m_LayoutHelper.indentLevel--;
                    break;
                }
                case EImageSourceType.RenderTexture:
                {
                    OnCustomInputGui();
                    break;
                }

                case EImageSourceType.TaggedCamera:
                {
                    m_LayoutHelper.indentLevel++;
                    m_LayoutHelper.AddStringProperty("Tags:", serializedObject, () => settingsObj.m_CameraTag);
                    m_LayoutHelper.indentLevel--;
                    break;
                }

                case EImageSourceType.MainCamera:
                    break;
            }

            base.OnInputGui();
        }

        protected override void OnEncodingGui()
        {
            base.OnOutputGui();
            var settingsObj = target as ImageRecorderSettings;

            switch (settingsObj.m_InputType)
            {
                case EImageSourceType.GameDisplay:
                {
                    m_LayoutHelper.AddBoolProperty("Scale image size", serializedObject, () => settingsObj.m_ScaleImage);

                    if (settingsObj.m_ScaleImage)
                    {
                        settingsObj.m_PreserveSourceAspectRatio = true;
                        m_LayoutHelper.AddIntProperty("Resolution (width)", serializedObject, () => settingsObj.m_Width);
                    }
                    break;
                }

                case EImageSourceType.MainCamera:
                case EImageSourceType.TaggedCamera:
                {
                    settingsObj.m_PreserveSourceAspectRatio = true;
                    m_LayoutHelper.AddIntProperty("Resolution (width)", serializedObject, () => settingsObj.m_Width);
                    break;
                }
            }
        }

        protected virtual void OnCustomInputGui()
        {
        }
    }
}
