using System;
using System.Linq.Expressions;
using UnityEngine;

namespace UnityEditor.Recorder.FrameRecorder.Utilities
{
    class LayoutHelper
    {
        public delegate T OnValueChangedDelegate<T>(T oldValue, T newValue);

        private string[] m_Indents = new[] { "", "   ", "      ", "        ",  "          "};
        GUILayoutOption m_LableLayoutOption;

        public LayoutHelper(GUILayoutOption lableLayoutOption)
        {
            m_LableLayoutOption = lableLayoutOption;
        }

        int m_IndentLevel;


        public int indentLevel
        {
            get { return m_IndentLevel; }
            set
            {
                m_IndentLevel = value;
                if (m_IndentLevel < 0) m_IndentLevel = 0;
                if (m_IndentLevel >= m_Indents.Length) m_IndentLevel = m_Indents.Length - 1;
            }
        }


        public string indentation
        {
            get { return m_Indents[m_IndentLevel]; }
        }

        public void AddPropertyLabel(string text)
        {
            GUILayout.Label(m_Indents[m_IndentLevel] + text, m_LableLayoutOption);
        }

        public void AddPropertyLabel(string label, string tooltip)
        {
            GUILayout.Label(new GUIContent(m_Indents[m_IndentLevel] + label, tooltip), m_LableLayoutOption);
        }

        public bool AddEnumProperty(string label, SerializedObject serObj, Expression<Func<object>> propertySelector, string tooltip = "")
        {
            var property = serObj.FindProperty(propertySelector);

            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            AddPropertyLabel(label, tooltip);

            var selectionFromInspector = property.intValue;
            var actualSelected = EditorGUILayout.Popup(selectionFromInspector, property.enumDisplayNames);
            var changed = actualSelected != property.intValue;
            property.intValue = actualSelected;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }

        public bool AddFlagEnumProperty(string label, SerializedObject serObj, Expression<Func<object>> propertySelector, string tooltip = "")
        {
            var property = serObj.FindProperty(propertySelector);

            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            AddPropertyLabel(label, tooltip);

            var selectionFromInspector = (int)Math.Log(property.intValue, 2);
            var actualSelected = 1 << EditorGUILayout.Popup(selectionFromInspector, property.enumDisplayNames);
            var changed = actualSelected != property.intValue;
            property.intValue = actualSelected;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }

        public bool AddEnumProperty(string label, SerializedObject serObj, Expression<Func<object>> propertySelector, string[] values, string tooltip = "")
        {
            var property = serObj.FindProperty(propertySelector);

            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            AddPropertyLabel(label, tooltip);

            var selectionFromInspector = property.intValue;
            var actualSelected = EditorGUILayout.Popup(selectionFromInspector, values);
            var changed = actualSelected != property.intValue;
            property.intValue = actualSelected;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }

        public bool AddStringProperty(string label, SerializedObject serObj, Expression<Func<object>> propertySelector, string tooltip = "")
        {
            var property = serObj.FindProperty(propertySelector);

            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            AddPropertyLabel(label);

            var orgValue = property.stringValue;
            var newValue = EditorGUILayout.TextField(new GUIContent("", tooltip), orgValue);
            var changed = orgValue != newValue;
            property.stringValue = newValue;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }

        public bool AddIntProperty(string label, SerializedProperty property, string tooltip = "")
        {
            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            AddPropertyLabel(label);

            var orgValue = property.intValue;
            var newValue = EditorGUILayout.IntField(new GUIContent("", tooltip), orgValue);
            var changed = orgValue != newValue;
            property.intValue = newValue;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }

        public bool AddIntProperty(string label, SerializedObject serObj, Expression<Func<object>> propertySelector, string tooltip = "")
        {
            return AddIntProperty(label, serObj.FindProperty(propertySelector), tooltip);
        }

        public bool AddIntProperty(SerializedProperty property)
        {
            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            var orgValue = property.intValue;
            var newValue = EditorGUILayout.IntField(orgValue);
            var changed = orgValue != newValue;
            property.intValue = newValue;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }

        public bool AddFloatProperty(SerializedObject serObj, Expression<Func<object>> propertySelector)
        {
            return AddFloatProperty(serObj.FindProperty(propertySelector));
        }

        public bool AddFloatProperty(SerializedProperty property)
        {
            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            var orgValue = property.floatValue;
            var newValue = EditorGUILayout.FloatField(orgValue);
            var changed = orgValue != newValue;
            property.floatValue = newValue;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }

        public bool AddIntProperty(SerializedObject serObj, Expression<Func<object>> propertySelector)
        {
            return AddIntProperty(serObj.FindProperty(propertySelector));
        }

        public bool AddIntProperty(SerializedProperty parentSerProp, Expression<Func<object>> propertySelector)
        {
            return AddIntProperty(parentSerProp.FindPropertyRelative(propertySelector));
        }

        public bool AddFloatProperty(string label, SerializedObject serObj, Expression<Func<object>> propertySelector, string tooltip = "")
        {
            var property = serObj.FindProperty(propertySelector);

            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            AddPropertyLabel(label);

            var orgValue = property.floatValue;
            var newValue = EditorGUILayout.FloatField(new GUIContent("", tooltip), orgValue);
            var changed = orgValue != newValue;
            property.floatValue = newValue;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }

        public bool AddDoubleProperty(string label, SerializedObject serObj, Expression<Func<object>> propertySelector, string tooltip = "")
        {
            var property = serObj.FindProperty(propertySelector);

            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            AddPropertyLabel(label);

            var orgValue = property.doubleValue;
            var newValue = EditorGUILayout.DoubleField(new GUIContent("", tooltip), orgValue);
            var changed = orgValue != newValue;
            property.doubleValue = newValue;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }

        public bool AddBoolProperty(string label, SerializedObject serObj, Expression<Func<object>> propertySelector, string tooltip = "")
        {
            var property = serObj.FindProperty(propertySelector);

            var ourRect = EditorGUILayout.BeginHorizontal();
            EditorGUI.BeginProperty(ourRect, GUIContent.none, property);

            AddPropertyLabel(label);

            var orgValue = property.boolValue;
            var newValue = EditorGUILayout.Toggle(new GUIContent("", tooltip), orgValue);
            var changed = orgValue != newValue;
            property.boolValue = newValue;

            EditorGUI.EndProperty();
            EditorGUILayout.EndHorizontal();

            return changed;
        }
    }
}
