import js from '@eslint/js';
import globals from 'globals';
import react from 'eslint-plugin-react';
import reactHooks from 'eslint-plugin-react-hooks';
import reactRefresh from 'eslint-plugin-react-refresh';
import tseslint from 'typescript-eslint';
import importPlugin from 'eslint-plugin-import';
import jsxA11y from 'eslint-plugin-jsx-a11y';
import sonarjs from 'eslint-plugin-sonarjs';
import security from 'eslint-plugin-security';
import unusedImports from 'eslint-plugin-unused-imports';
import unicorn from 'eslint-plugin-unicorn';
import functional from 'eslint-plugin-functional';
import perfectionist from 'eslint-plugin-perfectionist';

export default tseslint.config(
    { ignores: ['dist', 'node_modules'] },
    {
        extends: [
            js.configs.recommended,
            ...tseslint.configs.recommended,
        ],
        files: ['**/*.{ts,tsx}'],
        languageOptions: {
            ecmaVersion: 2022,
            sourceType: 'module',
            globals: globals.browser,
        },
        plugins: {
            react,
            'react-hooks': reactHooks,
            'react-refresh': reactRefresh,
            import: importPlugin,
            'jsx-a11y': jsxA11y,
            sonarjs,
            security,
            'unused-imports': unusedImports,
            unicorn,
            functional,
            perfectionist,
        },
        rules: {
            ...react.configs.recommended.rules,
            ...reactHooks.configs.recommended.rules,
            'react-refresh/only-export-components': ['warn', { allowConstantExport: true }],
            "react/react-in-jsx-scope": "off",
            "react/prop-types": "off",

            'import/order': ['warn', { 'newlines-between': 'never' }],
            'unused-imports/no-unused-imports': 'error',
            'unicorn/prefer-query-selector': 'warn',
            'perfectionist/sort-objects': ['warn', { order: 'asc' }],
        },
        settings: {
            react: {
                version: 'detect',
            },
        },
    },
);

