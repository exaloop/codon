import React from 'react';
import {useColorMode} from '@docusaurus/theme-common';
import OriginalColorModeToggle from '@theme-original/ColorModeToggle';

export default function ColorModeToggle(props) {
  const {colorMode} = useColorMode();
  return (
    <OriginalColorModeToggle
      {...props}
      className={['binary-color-toggle', props.className].filter(Boolean).join(' ')}
      value={colorMode}
      respectPrefersColorScheme={false}
    />
  );
}
