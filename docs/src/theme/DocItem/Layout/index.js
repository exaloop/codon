import React from 'react';
import {useDoc} from '@docusaurus/plugin-content-docs/client';
import OriginalDocItemLayout from '@theme-original/DocItem/Layout';
import DocumentationActions from '@site/src/components/DocumentationActions';

export default function DocItemLayout(props) {
  const {metadata} = useDoc();
  const showActions = metadata.id !== 'index' && !/^libraries\/api(?:\/|$)/.test(metadata.id);
  return <OriginalDocItemLayout {...props}>
    {showActions && <DocumentationActions key={metadata.permalink} />}
    {props.children}
  </OriginalDocItemLayout>;
}
