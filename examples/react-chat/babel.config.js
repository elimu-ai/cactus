module.exports = function(api) {
  api.cache(true);
  return {
    presets: ['babel-preset-expo'],
    plugins: [
      [
        'module-resolver',
        {
          alias: {
            // This needs to be mirrored in tsconfig.json
            '@': './',
          },
        },
      ],
      'react-native-reanimated/plugin', // Ensure reanimated plugin is last
    ],
  };
}; 