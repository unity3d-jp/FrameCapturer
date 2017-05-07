using System;

namespace UnityEngine.Recorder.FrameRecorder
{
    public class FrameRecorderGOControler
    {
        const string k_HostGoName = "UnityEngine.Recorder.FrameRecorder";

        public static GameObject GetGameObject()
        {
            return GameObject.Find(k_HostGoName) ?? new GameObject(k_HostGoName);
        }

        public static GameObject GetSettingsRoot()
        {
            var root = GetGameObject();
            var settingsTr = root.transform.Find("Settings");
            GameObject settingsGO;
            if (settingsTr == null)
            {
                settingsGO = new GameObject("Settings");
                settingsGO.transform.parent = root.transform;
            }
            else
                settingsGO = settingsTr.gameObject;

            return settingsGO;
        }

        public static GameObject GetRecordersRoot()
        {
            var root = GetGameObject();
            var settingsTr = root.transform.Find("Recording");
            GameObject settingsGO;
            if (settingsTr == null)
            {
                settingsGO = new GameObject("Recording");
                settingsGO.transform.parent = root.transform;
            }
            else
                settingsGO = settingsTr.gameObject;

            return settingsGO;
        }

        public static GameObject HookupRecorder(FrameRecorderSettings settings)
        {
            var ctrl = GetRecordersRoot();

            var recorderGO = new GameObject(settings.m_UniqueID);
            recorderGO.transform.parent = ctrl.transform;

            return recorderGO;
        }

        public static GameObject FindRecorder(FrameRecorderSettings settings)
        {
            var ctrl = GetRecordersRoot();
            var go = ctrl.transform.Find(settings.m_UniqueID);
            return go == null ? null : go.gameObject;
        }
    }
}
