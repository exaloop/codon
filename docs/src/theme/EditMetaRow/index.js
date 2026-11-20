import React from 'react';
import OriginalEditMetaRow from '@theme-original/EditMetaRow';

export default function EditMetaRow(props) {
  if (!props.lastUpdatedAt && !props.lastUpdatedBy) return null;
  return <OriginalEditMetaRow {...props} editUrl={undefined} />;
}
