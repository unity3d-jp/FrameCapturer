using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.Recorder.FrameRecorder;

namespace Assets.Unity.FrameRecorder.Scripts.Editor
{
    interface IRecorderSelectorTarget
    {
        string recorderCategory { get; set; }
        string selectedRecorder { get;  }
        void SetRecorder(Type newRecorderType);
    }

    class RecorderSelector
    {
        IRecorderSelectorTarget m_target;
        string[] categoryRecorders;

        public RecorderSelector(IRecorderSelectorTarget target)
        {
            m_target = target;
        }

        int GetCategoryIndex()
        {
            var categories = RecordersInventory.availableCategories;
            for (int i = 0; i < categories.Length; i++)
                if (categories[i] == m_target.recorderCategory)
                    return i;

            if (categories.Length > 0)
                return 0;
            else
                return -1;
        }

        bool SetCategoryFromIndex(int index)
        {
            if (index >= 0)
            {
                m_target.recorderCategory = RecordersInventory.availableCategories[index];
                categoryRecorders = RecordersInventory.recordersByCategory[m_target.recorderCategory]
                    .Select(x => x.displayName)
                    .ToArray();
            }
            else
            {
                m_target.recorderCategory = string.Empty;
                categoryRecorders = new string[0];
            }

            return index >= 0;
        }

        int GetRecorderIndex()
        {
            if (!RecordersInventory.recordersByCategory.ContainsKey(m_target.recorderCategory))
                return -1;

            var categoryRecorders = RecordersInventory.recordersByCategory[m_target.recorderCategory];
            for (int i = 0; i < categoryRecorders.Count; i++)
                if (categoryRecorders[i].recorder.AssemblyQualifiedName == m_target.selectedRecorder)
                    return i;


            if (categoryRecorders.Count > 0)
                return 0;
            else
                return -1;
        }

        Type GetRecorderFromIndex(int index)
        {
            if (index >= 0)
                return RecordersInventory.recordersByCategory[m_target.recorderCategory][index].recorder;

            return null;
        }

        public void OnGui()
        {
            // Group selection
            EditorGUILayout.BeginHorizontal();
            SetCategoryFromIndex(EditorGUILayout.Popup("Record what:", GetCategoryIndex(), RecordersInventory.availableCategories));
            EditorGUILayout.EndHorizontal();

            // Recorder in group selection
            EditorGUILayout.BeginHorizontal();
            var newIndex = EditorGUILayout.Popup("Using Recorder:", GetRecorderIndex(), categoryRecorders);
            m_target.SetRecorder(GetRecorderFromIndex(newIndex));
            EditorGUILayout.EndHorizontal();
        }
    }
}
