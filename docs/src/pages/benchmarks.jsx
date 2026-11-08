import React, {useEffect, useId, useRef, useState} from 'react';
import Layout from '@theme/Layout';
import CodeBlock from '@theme/CodeBlock';
import Link from '@docusaurus/Link';
import {useLocation} from '@docusaurus/router';
import {AlertCircle, ArrowUpRight, Check, Copy, Download, Info, Link2, Search, X} from 'lucide';
import styles from './benchmarks.module.css';

const datasetFiles = require.context('../../data/benchmarks', true, /^\.\/(?:aws\/)?results\.json$/);
const datasets = Object.fromEntries(datasetFiles.keys().map(filename => [filename === './results.json' ? 'mac' : 'aws', datasetFiles(filename)]));
const macData = datasets.mac;
const machineLabels = {mac: 'Apple M1 Max / macOS', aws: 'AWS Xeon / Linux / Tesla T4'};
const machines = Object.keys(machineLabels).filter(machine => Object.hasOwn(datasets, machine));
const suites = {applications: 'Applications', pyperformance: 'Pyperformance', npbench: 'NPBench'};
const tools = ['codon', 'python', 'pypy', 'cpp', 'numba'];
const states = {timeout: 'Timed out', 'compile-error': 'Compile error', error: 'Execution failed'};
const number = new Intl.NumberFormat('en-US', {maximumSignificantDigits: 4});
const sourceCache = new Map();

function isMeasured(result) {
  return Number.isFinite(result?.median);
}

function firstBenchmark(data) {
  return data.cases.find(benchmark => benchmark.suite === 'applications') || data.cases[0];
}

function comparisonSpeedup(benchmark, tool) {
  const native = benchmark.results.codon?.median;
  const reference = benchmark.results[tool]?.median;
  return Number.isFinite(native) && native > 0 && Number.isFinite(reference) && reference > 0 ? reference / native : null;
}

function summarizeSuite(benchmarks) {
  const comparisons = tools.filter(tool => tool !== 'codon' && benchmarks.some(benchmark => benchmark.results[tool])).map(tool => {
    const ratios = benchmarks.map(benchmark => comparisonSpeedup(benchmark, tool)).filter(value => value !== null);
    return {tool, label: benchmarks.find(benchmark => benchmark.results[tool]).results[tool].label,
      count: ratios.length, geomean: ratios.length ? Math.exp(ratios.reduce((sum, value) => sum + Math.log(value), 0) / ratios.length) : null};
  });
  const logarithms = benchmarks.flatMap(benchmark => comparisons.map(({tool}) => comparisonSpeedup(benchmark, tool)).filter(value => value !== null).map(Math.log10));
  const minimum = Math.floor(Math.min(0, ...logarithms));
  const maximum = Math.max(minimum + 1, Math.ceil(Math.max(0, ...logarithms)));
  return {comparisons, minimum, maximum};
}

function Icon({icon}) {
  return <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    {icon.map(([tag, attributes], index) => React.createElement(tag, {...attributes, key: index}))}
  </svg>;
}

function BenchmarkNotes({benchmark}) {
  const id = useId();
  const trigger = useRef(null);
  const tooltip = useRef(null);
  const closeTimer = useRef(null);
  const notes = [benchmark.outputNote, benchmark.threadingNote, benchmark.acceleratorNote].filter(Boolean);
  function hide() {
    clearTimeout(closeTimer.current);
    if (tooltip.current?.matches(':popover-open')) tooltip.current.hidePopover();
  }
  function show() {
    clearTimeout(closeTimer.current);
    const popup = tooltip.current;
    if (!popup || popup.matches(':popover-open')) return;
    popup.showPopover();
    const anchor = trigger.current.getBoundingClientRect();
    const bounds = popup.getBoundingClientRect();
    popup.style.left = `${Math.max(12, Math.min(anchor.left, window.innerWidth - bounds.width - 12))}px`;
    popup.style.top = `${anchor.bottom + bounds.height + 8 <= window.innerHeight - 12 ? anchor.bottom + 8 : Math.max(12, anchor.top - bounds.height - 8)}px`;
  }
  useEffect(() => {
    if (!tooltip.current) return;
    function dismiss(event) {
      if (!tooltip.current?.contains(event.target)) hide();
    }
    function dismissKey(event) {
      if (event.key === 'Escape') hide();
    }
    window.addEventListener('scroll', dismiss, true);
    window.addEventListener('resize', hide);
    window.addEventListener('keydown', dismissKey);
    return () => {
      clearTimeout(closeTimer.current);
      window.removeEventListener('scroll', dismiss, true);
      window.removeEventListener('resize', hide);
      window.removeEventListener('keydown', dismissKey);
    };
  }, [benchmark]);
  if (!notes.length) return null;
  return <span className={styles.benchmarkNotes} onPointerEnter={event => { if (event.pointerType === 'mouse') show(); }} onPointerLeave={event => { if (event.pointerType === 'mouse') closeTimer.current = setTimeout(hide, 150); }} onBlur={event => { if (!event.currentTarget.contains(event.relatedTarget)) hide(); }}>
    <button ref={trigger} type="button" className={styles.noteTrigger} aria-label={`Notes for ${benchmark.name}`} aria-describedby={id} onFocus={show} onClick={show}><Icon icon={Info} /></button>
    <span ref={tooltip} id={id} role="tooltip" popover="auto" className={styles.noteTooltip} onPointerEnter={() => clearTimeout(closeTimer.current)}><strong>{benchmark.name}</strong>{notes.map(note => <span key={note}>{note}</span>)}</span>
  </span>;
}

function version(tool, data) {
  const text = data.versions[tool === 'cpp' ? 'clang' : tool];
  if (tool === 'pypy') return text.match(/PyPy ([\d.]+)/)?.[1] || text;
  return text.match(/\d+\.\d+\.\d+/)?.[0] || text;
}

function duration(seconds) {
  if (seconds === 0) return '0 s';
  if (seconds >= 1) return `${number.format(seconds)} s`;
  if (seconds >= 0.001) return `${number.format(seconds * 1000)} ms`;
  return `${number.format(seconds * 1000000)} us`;
}

function loadSource(result) {
  const url = result.displaySourceUrl || result.sourceUrl;
  if (!sourceCache.has(url)) {
    const entry = {};
    entry.promise = fetch(url).then(response => {
      if (!response.ok || response.headers.get('content-type')?.includes('text/html')) throw new Error('Source unavailable');
      return response.text();
    }).then(text => {
      entry.source = {url, text, language: result.language, error: false};
      return entry.source;
    }).catch(() => {
      sourceCache.delete(url);
      return {url, text: '', error: true};
    });
    sourceCache.set(url, entry);
  }
  return sourceCache.get(url).promise;
}

function TabList({label, options, value, onChange, onIntent}) {
  function navigate(event, index) {
    const direction = {ArrowRight: 1, ArrowLeft: -1}[event.key];
    const next = event.key === 'Home' ? 0 : event.key === 'End' ? options.length - 1 : direction ? (index + direction + options.length) % options.length : null;
    if (next === null) return;
    event.preventDefault();
    onChange(options[next].value);
    event.currentTarget.parentElement.querySelectorAll('[role="tab"]')[next].focus({preventScroll: true});
  }
  return <div className={styles.tabs} role="tablist" aria-label={label}>
    {options.map((option, index) => <button key={option.value} type="button" role="tab" aria-selected={value === option.value}
      tabIndex={value === option.value ? 0 : -1} onClick={() => onChange(option.value)} onKeyDown={event => navigate(event, index)}
      onPointerEnter={() => onIntent?.(option.value)} onFocus={() => onIntent?.(option.value)}>{option.label}</button>)}
  </div>;
}

function BenchmarkSummary({data, machine}) {
  const assetBase = data.assetBase || '/benchmarks';
  function workloadUrl(benchmark) {
    return `/benchmarks/?${new URLSearchParams({machine, benchmark: benchmark.id, tool: 'codon', view: 'source'})}`;
  }
  return <section className={styles.overview} aria-label="Benchmark summary">
    <div className={styles.overviewHeading}><div><h2>Codon speedups</h2><p>Comparison runtime / Codon runtime. Higher than 1x favors Codon.</p></div><a className={styles.summaryDownload} href={`${assetBase}/results.json`} download title="Download results JSON" aria-label="Download summary results JSON"><Icon icon={Download} /> Results JSON</a></div>
    {Object.entries(suites).map(([suite, label], index) => {
      const benchmarks = data.cases.filter(benchmark => benchmark.suite === suite);
      if (!benchmarks.length) return null;
      const {comparisons, minimum, maximum} = summarizeSuite(benchmarks);
      const position = value => 4 + 92 * (Math.log10(value) - minimum) / (maximum - minimum);
      const parity = position(1);
      const ticks = Array.from(new Set([minimum, 0, maximum])).sort((first, second) => first - second);
      return <section className={styles.suiteSummary} key={suite} aria-labelledby={`summary-${suite}`} style={{'--comparison-count': comparisons.length, '--row-count': benchmarks.length}}>
        <div className={styles.suiteHeading}><div className={styles.suiteIdentity}><span className={styles.suiteIndex}>{String(index + 1).padStart(2, '0')}</span><div><h2 id={`summary-${suite}`}>{label}</h2><p>{benchmarks.length} workloads / {suite === 'npbench' ? 'Largest standard L preset' : 'Full application inputs'}</p></div></div><Link to={workloadUrl(benchmarks[0])} className={styles.exploreSuite} title={`Explore ${label} workloads`} aria-label={`Explore ${label} workloads`}>Explore workloads <Icon icon={ArrowUpRight} /></Link></div>
        <p className={styles.geomeanLabel}>Geometric mean speedup</p>
        <dl className={styles.geomeans}>{comparisons.map(({tool, label: toolLabel, count, geomean}) => <div key={tool} data-tool={tool}>
          <dt><span className={styles.toolSwatch} />vs. {toolLabel}<span className={styles.summaryVersion}>{version(tool, data)}</span></dt>
          <dd><strong>{geomean === null ? 'N/A' : `${number.format(geomean)}x`}</strong><span>{count} / {benchmarks.length} workloads</span></dd>
        </div>)}</dl>
        <figure className={styles.suiteFigure} aria-label={`${label}: Codon speedups across all workloads`}>
          <div className={styles.summaryPlot} role="region" aria-label={`${label} speedup chart`} tabIndex={0}>
            <table className={styles.summaryTable}><thead><tr><th scope="col">Workload<span className={styles.scaleLabel}>Logarithmic scale / 1x = parity</span></th>{comparisons.map(({tool, label: toolLabel}) => <th scope="col" key={tool} data-tool={tool}><span className={styles.comparisonHeading}><span className={styles.toolSwatch} />vs. {toolLabel}</span><div className={styles.summaryCell}><div className={styles.summaryScale}>{ticks.map(exponent => <span key={exponent} style={{left: `${position(10 ** exponent)}%`}}>{number.format(10 ** exponent)}x</span>)}</div><span /></div></th>)}</tr></thead>
              <tbody>{benchmarks.map(benchmark => <tr key={benchmark.id}><th scope="row"><div className={styles.benchmarkLabel}><Link to={workloadUrl(benchmark)} title={benchmark.title}>{benchmark.name}<Icon icon={ArrowUpRight} /></Link><BenchmarkNotes benchmark={benchmark} /></div></th>{comparisons.map(({tool, label: toolLabel}) => {
                const ratio = comparisonSpeedup(benchmark, tool);
                const endpoint = ratio === null ? parity : position(ratio);
                return <td key={tool} data-tool={tool}>{ratio === null ? <span className={styles.missingRatio}>{benchmark.results[tool] ? states[benchmark.results[tool].status] || 'Not measured' : 'Not implemented'}</span> : <div className={styles.summaryCell} title={`${benchmark.name}: ${number.format(ratio)}x vs. ${toolLabel}`}>
                  <div className={styles.speedupTrack} aria-hidden="true"><span className={styles.parityLine} style={{left: `${parity}%`}} /><span className={styles.speedupBar} style={{left: `${Math.min(parity, endpoint)}%`, width: `${Math.abs(endpoint - parity)}%`}} /><span className={styles.speedupDot} style={{left: `${endpoint}%`}} /></div><span className={styles.ratioValue}>{number.format(ratio)}x</span>
                </div>}</td>;
              })}</tr>)}</tbody>
            </table>
          </div>
          <figcaption>Geometric means of paired median-time ratios; {data.sampleCount} measured runs per implementation. Missing or incomplete timings are excluded.</figcaption>
        </figure>
      </section>;
    })}
  </section>;
}

function Chart({benchmark, mode, baseline, logarithmic, data}) {
  const entries = tools.filter(tool => benchmark.results[tool]).map(tool => [tool, benchmark.results[tool]]);
  const reference = benchmark.results[baseline];
  const valid = entries.filter(([, result]) => isMeasured(result));
  const values = valid.flatMap(([, result]) => mode === 'time' ? [result.minimum, result.maximum] : [reference.median / result.maximum, reference.median / result.minimum]);
  const minimum = logarithmic ? 10 ** Math.floor(Math.log10(Math.min(...values))) : 0;
  const maximum = values.length ? (logarithmic ? 10 ** Math.ceil(Math.log10(Math.max(...values) * 1.01)) : Math.max(...values) * 1.08) : 1;
  function position(value) {
    return logarithmic ? 100 * Math.log(value / minimum) / Math.log(maximum / minimum) : 100 * value / maximum;
  }
  const format = value => mode === 'time' ? duration(value) : `${number.format(value)}x`;
  const ticks = Array.from({length: 5}, (_, index) => logarithmic ? minimum * (maximum / minimum) ** (index / 4) : maximum * index / 4);
  return <figure className={styles.chart} aria-label={`${benchmark.title}: ${mode === 'time' ? 'median runtime, lower is better' : 'speedup, higher is better'}`}>
    <div className={styles.chartHeading}><span>{mode === 'time' ? 'Median wall time' : `Speedup relative to ${reference?.label || baseline}`}</span><span>{mode === 'time' ? 'Lower is better' : 'Higher is better'}</span></div>
    {entries.map(([tool, result]) => {
      const okay = isMeasured(result);
      const median = okay ? (mode === 'time' ? result.median : reference.median / result.median) : 0;
      const low = okay ? (mode === 'time' ? result.minimum : reference.median / result.maximum) : 0;
      const high = okay ? (mode === 'time' ? result.maximum : reference.median / result.minimum) : 0;
      return <div className={styles.chartRow} key={tool} data-tool={tool}>
        <div className={styles.chartLabel}><strong>{result.label}</strong><span>{version(tool, data)}</span></div>
        {okay ? <>
          <div className={styles.track} aria-hidden="true">
            <div className={styles.bar} style={{width: `${position(median)}%`}} />
            <div className={styles.range} style={{left: `${position(low)}%`, width: `${position(high) - position(low)}%`}} />
          </div>
          <div className={styles.chartValue}><strong>{format(median)}</strong><span>{format(low)} - {format(high)}</span></div>
        </> : <div className={styles.withheld}><Icon icon={AlertCircle} /><span><strong>{states[result.status] || 'Unavailable'}</strong><small>{result.status === 'timeout' ? `Process exceeded ${result.timeoutSeconds} seconds.` : result.reason || 'No comparable result was recorded.'}</small></span></div>}
      </div>;
    })}
    {valid.length > 0 && <div className={styles.axis} aria-hidden="true"><div /><div>{ticks.map((tick, index) => <span key={index} style={{left: `${index * 25}%`}}>{format(tick)}</span>)}</div><div /></div>}
    <figcaption>Median of {data.sampleCount} measured runs. Whiskers show the observed minimum and maximum, not a confidence interval.{logarithmic && ' Logarithmic axis.'}</figcaption>
  </figure>;
}

function Setup({benchmark, tool, result, data}) {
  const native = tool === 'codon';
  return <div className={styles.setup}>
    <dl className={styles.facts}>
      <div><dt>Implementation</dt><dd>{result.label} {version(tool, data)}{tool === 'numba' && ` / CPython ${data.versions.python}`}</dd></div>
      <div><dt>Measured</dt><dd>{result.measurementDate || benchmark.measurementDate || data.date}</dd></div>
      <div><dt>Configuration</dt><dd>{benchmark.suite === 'npbench' ? 'NPBench L (largest standard preset)' : 'Full application workload'}</dd></div>
      {tool === 'numba' && result.fastmath != null && <div><dt>Fast math</dt><dd>{result.fastmath ? 'Enabled' : 'Disabled in kernels and helpers'}</dd></div>}
      {benchmark.inputSeed != null && <div><dt>Input seed</dt><dd>{benchmark.inputSeed} (NumPy legacy RNG; shared across runtimes)</dd></div>}
      <div><dt>Timing boundary</dt><dd>{benchmark.policy}</dd></div>
      <div><dt>Output check</dt><dd>{result.validation?.status === 'passed' ? 'Passed' : result.status === 'invalid' ? 'Output differs between runtimes' : isMeasured(result) ? 'Not recorded' : 'Not completed'}{result.validation?.elements != null && `; ${number.format(result.validation.elements)} array elements checked`}</dd></div>
      {result.parallelThreads != null && <div><dt>OpenMP threads</dt><dd>{result.parallelThreads}{result.parallelThreads > 1 ? ' (explicit in the source)' : ' (environment setting)'}</dd></div>}
      {result.execution === 'gpu' && <div><dt>Accelerator</dt><dd>{data.environment.gpu}</dd></div>}
      {result.hugePagesKiB != null && <div><dt>Huge pages</dt><dd>Enabled (always); {number.format(result.hugePagesKiB / 1024)} MiB observed before timing.</dd></div>}
      <div><dt>Source SHA-256</dt><dd className={styles.hash}>{result.sourceSha256}</dd></div>
    </dl>
    {Object.keys(benchmark.parameters).length > 0 && <><h3>Parameters</h3><div className={styles.parameters}>{Object.entries(benchmark.parameters).map(([name, value]) => <span key={name}><code>{name}</code> {String(value)}</span>)}</div></>}
    {benchmark.arguments.length > 0 && <><h3>Arguments</h3><code className={styles.wrapCode}>{benchmark.arguments.join(' ')}</code></>}
    {Object.keys(benchmark.inputs).length > 0 && <details className={styles.details}><summary>Input arrays and scalar values</summary><div className={styles.tableScroll}><table><thead><tr><th>Argument</th><th>Type</th><th>Shape / value</th></tr></thead><tbody>{Object.entries(benchmark.inputs).map(([name, input]) => <tr key={name}><td><code>{name}</code></td><td>{input.dtype || 'scalar'}</td><td>{input.shape?.join(' x ') || String(input.value)}</td></tr>)}</tbody></table></div></details>}
    {result.compileCommand && <><h3>Build command</h3><pre className={styles.command}>{result.compileCommand.map(argument => /\s/.test(argument) ? JSON.stringify(argument) : argument).join(' ')}</pre></>}
    {result.runtime && <><h3>Numerical libraries</h3><ul>{result.runtime.map((pool, index) => <li key={index}>{pool.internal_api} {pool.version}, {pool.num_threads} thread{pool.num_threads === 1 ? '' : 's'}<br /><code className={styles.wrapCode}>{pool.filepath}</code></li>)}</ul></>}
    {benchmark.suite === 'npbench' && native && <details className={styles.details}><summary>Memory observations</summary><p className={styles.small}>{result.memoryAudit}. Reset buffers retain their addresses; collection and reset are outside kernel timing.</p>{result.gc && <p className={styles.small}>Post-collection live growth: {number.format(result.gc.liveGrowthBytes / 1048576)} MiB. Full observations are retained in the raw report.</p>}</details>}
    {(result.adaptations || []).length > 0 && <><h3>Recorded adaptations</h3><ul>{result.adaptations.map(note => <li key={note}>{note}</li>)}</ul></>}
    {result.workerUrl && <a href={result.workerUrl} download>Download the measurement harness (paths redacted)</a>}
  </div>;
}

function SourcePane({benchmark, tool, view, setTool, setView, data}) {
  const result = benchmark.results[tool];
  const [loadedSource, setLoadedSource] = useState({url: '', text: '', error: false});
  const [copyState, setCopyState] = useState('');
  const copyGeneration = useRef(0);
  const url = result.displaySourceUrl || result.sourceUrl;
  const source = sourceCache.get(url)?.source || loadedSource;
  const loading = source.url !== url;
  useEffect(() => {
    let active = true;
    copyGeneration.current++;
    setCopyState('');
    loadSource(result).then(next => { if (active) setLoadedSource(next); });
    return () => { active = false; copyGeneration.current++; };
  }, [result]);
  async function copySource() {
    const generation = ++copyGeneration.current;
    try { await navigator.clipboard.writeText(source.text); if (generation === copyGeneration.current) setCopyState('Copied'); }
    catch { if (generation === copyGeneration.current) setCopyState('Copy unavailable'); }
  }
  return <section className={styles.inspector} aria-label="Benchmark implementation">
    <TabList label="Benchmark details" options={[{value: 'source', label: 'Source'}, {value: 'setup', label: 'Setup'}, {value: 'samples', label: 'Samples'}]} value={view} onChange={setView} />
    <div className={styles.toolRow}>
      <TabList label="Implementation" options={tools.filter(key => benchmark.results[key]).map(key => ({value: key, label: benchmark.results[key].label}))} value={tool} onChange={setTool} onIntent={key => loadSource(benchmark.results[key])} />
      <span className={styles.toolVersion}>{result.label} {version(tool, data)}</span>
    </div>
    <div className={`${styles.detailPanel} ${styles.sourcePanel}`} hidden={view !== 'source'} role="tabpanel" aria-label={`${result.label} source`}>
      <div className={styles.sourceToolbar}><span>{result.displaySourceUrl ? 'Kernel excerpt' : 'Executed source'}</span><div className={styles.actions}>
        <span role="status">{copyState}</span>
        <button type="button" title="Copy displayed source" aria-label="Copy displayed source" onClick={copySource} disabled={loading || !source.text}><Icon icon={copyState === 'Copied' ? Check : Copy} /></button>
        <a href={result.sourceUrl} download title={result.displaySourceUrl ? 'Download full executed library' : 'Download executed source'} aria-label="Download executed source"><Icon icon={Download} /></a>
      </div></div>
      <div className={styles.sourceFrame}>
        <div className={styles.code} aria-busy={loading}>
          <div inert={loading} aria-hidden={loading || undefined}>
            {source.text ? <CodeBlock language={source.language} showLineNumbers>{source.text}</CodeBlock> : !loading && source.error ? <p className={styles.sourceError} role="alert">Source could not be loaded. <a href={result.sourceUrl}>Open source file</a></p> : <div className={styles.sourcePlaceholder} />}
          </div>
        </div>
        {loading && <div className={styles.sourceLoading} role="status">Loading source...</div>}
      </div>
      {result.displaySourceUrl && <p className={styles.sourceNote}>Excerpt from the executed native library. <a href={result.sourceUrl} download>Full library</a></p>}
    </div>
    <div className={styles.detailPanel} hidden={view !== 'setup'} role="tabpanel" aria-label={`${result.label} setup`}>
      <Setup benchmark={benchmark} tool={tool} result={result} data={data} />
    </div>
    <div className={styles.detailPanel} hidden={view !== 'samples'} role="tabpanel" aria-label="Recorded samples">
      <div className={styles.samplePane}><p className={styles.small}>Recorded wall time in seconds. Warm-up is excluded; incomplete runs have no reported timing.</p><div className={styles.tableScroll}><table><thead><tr><th>Implementation</th>{Array.from({length: data.sampleCount}, (_, index) => <th key={index}>Run {index + 1}</th>)}<th>Median</th></tr></thead><tbody>{tools.filter(key => benchmark.results[key]).map(key => {
        const item = benchmark.results[key];
        return <tr key={key}><th scope="row">{item.label}</th>{isMeasured(item) ? <>{item.samples.map((sample, index) => <td key={index}>{sample.toPrecision(6)}</td>)}<td>{item.median.toPrecision(6)}</td></> : <td colSpan={data.sampleCount + 1}>{states[item.status] || 'Unavailable'}</td>}</tr>;
      })}</tbody></table></div></div>
    </div>
  </section>;
}

function Methodology({data}) {
  const environment = data.environment;
  const linux = environment.platform === 'linux';
  const assetBase = data.assetBase || '/benchmarks';
  const unavailable = data.cases.flatMap(item => Object.values(item.results)).filter(result => !isMeasured(result)).length;
  return <details className={styles.methodology} id="methodology"><summary>Methodology and reproducibility</summary>
    <div className={styles.methodGrid}><section><h2>One machine, two timing scopes</h2><p>{environment.cpu}, {number.format(environment.memoryBytes / 1073741824)} GiB RAM, {linux ? `${environment.os}, kernel ${environment.kernel}` : `macOS ${environment.os} (${environment.macosBuild})`}, {environment.logicalCpus} logical CPUs. OpenMP and numerical libraries default to one worker. Codon's Fannkuch explicitly requests four OpenMP threads; its parallel Primes loop uses the one-thread default. Native compilation and input preparation finished before the serial timing sweep.</p>
      <p>Pyperformance and Applications retain their original timing boundaries. Each measurement uses a fresh process, so allocation and any PyPy JIT work inside the timer are included. One preliminary process is excluded. NPBench uses one process per implementation, one warm-up, then three kernel-only measurements; Numba specialization, input reset, and output serialization are excluded.</p>
      <p>These are software-stack comparisons, not isolated compiler comparisons. Native Codon links {linux ? 'OpenBLAS ILP64' : 'Apple Accelerate'}; NumPy and Numba use the libraries recorded under Setup. {data.npbenchRefresh ? 'Numba fast math is disabled in every kernel and helper. Codon is compiled without -fast-math. Other upstream Numba options are retained.' : 'Upstream Numba flags, including fast math, are retained.'} Results describe this hardware and these inputs, not a universal ranking.</p>
      {data.npbenchRefresh && <p>Numba was remeasured on {data.npbenchRefresh.date}, using the original input hashes for 52 kernels. MLP was remeasured on all three runtimes: its previously unseeded input now uses NumPy legacy RNG seed 42, shared across runtimes; its seeded weights are unchanged. {data.histogramRefresh ? `A separate histogram update on ${data.histogramRefresh.date} uses the uniform implementation on all three runtimes. All other Codon and Python timings are retained from the original sweep.` : 'Other Codon and Python timings are retained from the original sweep.'}</p>}
      <h3>Memory and output checks</h3><p>NPBench uses identical initialized inputs and persistent pristine/working arrays. Working arrays are reset in place, with garbage collection outside the timer. A separate reset audit checks allocation and address stability. Live-heap observations are retained in the raw report and do not determine whether completed timings are displayed.</p>
      {linux && <p>Linux transparent huge pages were explicitly set to <code>always</code>, and <code>NUMPY_MADVISE_HUGEPAGE=1</code> was set for NumPy and Numba. The policy is checked before and after the run. Each NPBench worker records its anonymous huge-page usage before timing; the observed amount is shown under Setup.</p>}
      <p>NPBench output checks use upstream pointwise tolerances or a relative L2-error threshold, with exact integer checks. Application values are compared with CPython; Chaos uses an exact PPM check. Completed timings are included when outputs differ, with a brief note on the affected benchmark. Rounding and the order of floating-point operations can affect iterative calculations; the original check results remain available in the raw report.</p>
      <p>The process timeout is {data.processTimeoutSeconds} seconds, including warm-up and output writing. {unavailable ? `${unavailable} incomplete implementation run${unavailable === 1 ? ' has' : 's have'} no reported timing; the diagnostics are retained in the raw report.` : 'All implementation runs have recorded timings.'}</p>
    </section><section><h2>Recorded environment</h2><dl className={styles.environment}>{Object.entries({Codon: version('codon', data), CPython: version('python', data), PyPy: `${version('pypy', data)} (Python ${data.versions.pypy.match(/Python ([\d.]+)/)?.[1] || 'unknown'})`, Clang: version('cpp', data), NumPy: data.versions.numpy, Numba: data.versions.numba, SciPy: data.versions.scipy, ...(linux ? {GPU: environment.gpu} : {'macOS SDK': environment.sdk})}).map(([name, value]) => <div key={name}><dt>{name}</dt><dd>{value}</dd></div>)}</dl>
      <p>{linux ? 'Clang uses native x86-64 CPU targeting and the system C++ runtime. Codon Mandelbrot uses the Tesla T4 GPU via @par(gpu=True); its timer includes transfers and launch overhead. The other Mandelbrot implementations and all NPBench kernels use the CPU.' : 'Clang uses the matching macOS SDK libc++ headers and system runtime. The CUDA-only Codon Mandelbrot variant is not measured on this host; the existing CPU implementation is used instead.'}</p>
      <h3>Source and inputs</h3><p>Codon commit <a href={`https://github.com/exaloop/codon/tree/${environment.codonCommit}`}><code>{environment.codonCommit.slice(0, 12)}</code></a>; local binary and source fingerprints are included in the raw report. NPBench commit <a href={`https://github.com/spcl/npbench/tree/${environment.npbenchCommit}`}><code>{environment.npbenchCommit.slice(0, 12)}</code></a>, always the largest standard <code>L</code> preset.</p>
      <p>TAQ uses the first 10 million lines of {data.datasets.taq.source}. Word count uses {number.format(data.datasets.words.bytes / 1048576)} MiB of {data.datasets.words.source}, ending at a complete line. Dataset hashes and array dimensions are recorded; external input files are not redistributed.</p>
      <p>The Numba MLP source corrects an upstream softmax maximum-axis error. {data.histogramRefresh ? "Azimuthal histogram integration uses the same portable weighted-histogram helpers across Codon, Python, and Numba. Python interprets the weighted loop; unweighted counts use each runtime's np.histogram. Each implementation has three measured samples." : "Azimuthal histogram integration retains the original per-runtime implementations, including Numba's portable weighted-histogram helper."} Other compatibility adaptations are listed per implementation. Exact executed kernel/application sources and raw samples accompany the results.</p>
      <div className={styles.resourceLinks}><a href={`${assetBase}/reproduction.md`} download>Reproduction guide</a><a href={`${assetBase}/raw-results.json`} download>Raw results and provenance</a><a href={`${assetBase}/NPBench-LICENSE.txt`}>NPBench license</a></div>
    </section></div>
  </details>;
}

export default function Benchmarks() {
  const location = useLocation();
  const [selection, setSelection] = useState({machine: 'mac', benchmark: firstBenchmark(macData).id, tool: 'codon', view: 'source', summary: true});
  const data = datasets[selection.machine];
  const assetBase = data.assetBase || '/benchmarks';
  const measuredCount = data.cases.reduce((count, item) => count + Object.values(item.results).filter(isMeasured).length, 0);
  const [query, setQuery] = useState('');
  const [suite, setSuite] = useState('all');
  const [mode, setMode] = useState('time');
  const [baseline, setBaseline] = useState('python');
  const [logarithmic, setLogarithmic] = useState(false);
  const [shareState, setShareState] = useState('');
  useEffect(() => {
    const params = new URLSearchParams(location.search);
    const machine = Object.hasOwn(datasets, params.get('machine')) ? params.get('machine') : 'mac';
    const selectedData = datasets[machine];
    const requestedBenchmark = selectedData.cases.find(item => item.id === params.get('benchmark'));
    const benchmark = requestedBenchmark || firstBenchmark(selectedData);
    const tool = Object.hasOwn(benchmark.results, params.get('tool')) ? params.get('tool') : Object.keys(benchmark.results)[0];
    const view = ['source', 'setup', 'samples'].includes(params.get('view')) ? params.get('view') : 'source';
    setSelection({machine, benchmark: benchmark.id, tool, view, summary: !requestedBenchmark});
    setShareState('');
  }, [location.search, location.key]);
  const benchmark = data.cases.find(item => item.id === selection.benchmark);
  const actualTool = Object.hasOwn(benchmark.results, selection.tool) ? selection.tool : Object.keys(benchmark.results)[0];
  const validTools = tools.filter(tool => isMeasured(benchmark.results[tool]));
  const actualBaseline = validTools.includes(baseline) ? baseline : validTools[0];
  const filtered = data.cases.filter(item => (suite === 'all' || item.suite === suite) && `${item.name} ${item.title}`.toLowerCase().includes(query.toLowerCase().trim()));
  function select(changes) {
    const next = {...selection, ...changes};
    const target = datasets[next.machine].cases.find(item => item.id === next.benchmark);
    const urlSelection = next.summary ? {machine: next.machine} : {machine: next.machine, benchmark: next.benchmark, view: next.view, tool: Object.hasOwn(target.results, next.tool) ? next.tool : Object.keys(target.results)[0]};
    setSelection(next);
    setShareState('');
    window.history.replaceState(window.history.state, '', `${location.pathname}?${new URLSearchParams(urlSelection)}${location.hash}`);
  }
  async function share() {
    try { await navigator.clipboard.writeText(window.location.href); setShareState('Link copied'); }
    catch { setShareState('Copy unavailable'); }
  }
  return <Layout title="Benchmarks" description="Reproducible Codon, Python, PyPy, C++ and Numba performance results with source code, raw measurements and setup details.">
    <main className={styles.page}>
      <header className={styles.header}><div><p className={styles.eyebrow}>Performance / {data.histogramRefresh?.date || data.npbenchRefresh?.date || data.date}</p><h1>Benchmarks</h1></div><div className={styles.headerMeta}><span>{data.environment.cpu}</span><span>{data.cases.length} workloads</span><span>{measuredCount} timings</span></div></header>
      <div className={styles.machineControls}><TabList label="Benchmark view" options={[{value: 'summary', label: 'Summary'}, {value: 'workloads', label: 'Workloads'}]} value={selection.summary ? 'summary' : 'workloads'} onChange={value => select({summary: value === 'summary'})} />{machines.length > 1 && <label>Machine <select aria-label="Benchmark machine" value={selection.machine} onChange={event => select({machine: event.target.value})}>{machines.map(machine => <option key={machine} value={machine}>{machineLabels[machine]}</option>)}</select></label>}</div>
      {selection.summary ? <BenchmarkSummary data={data} machine={selection.machine} /> : <div className={styles.workspace}>
        <aside className={styles.catalogue} aria-label="Benchmark catalogue"><div className={styles.catalogueHeading}><h2>Workloads</h2><span>{filtered.length}</span></div>
          <label className={styles.search}><Icon icon={Search} /><input type="search" value={query} onChange={event => setQuery(event.target.value)} placeholder="Search workloads" aria-label="Search workloads" />{query && <button type="button" onClick={() => setQuery('')} aria-label="Clear workload search" title="Clear search"><Icon icon={X} /></button>}</label>
          <label className={styles.suiteSelect}><span>Suite</span><select aria-label="Benchmark suite" value={suite} onChange={event => setSuite(event.target.value)}><option value="all">All suites</option>{Object.entries(suites).map(([key, label]) => <option key={key} value={key}>{label} ({data.cases.filter(item => item.suite === key).length})</option>)}</select></label>
          <nav className={styles.workloadList} aria-label="Workloads">{Object.entries(suites).map(([key, label]) => {
            const items = filtered.filter(item => item.suite === key);
            return items.length > 0 && <div key={key}><h3>{label}</h3>{items.map(item => <a key={item.id} href={`${location.pathname}?${new URLSearchParams({machine: selection.machine, benchmark: item.id, view: selection.view, tool: Object.hasOwn(item.results, selection.tool) ? selection.tool : Object.keys(item.results)[0]})}`} title={item.name}
              onPointerEnter={() => loadSource(item.results[selection.tool] || Object.values(item.results)[0])} onFocus={() => loadSource(item.results[selection.tool] || Object.values(item.results)[0])} onClick={event => {
              if (event.button !== 0 || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
              event.preventDefault();
              select({benchmark: item.id});
            }} className={selection.benchmark === item.id ? styles.selectedWorkload : undefined} aria-current={selection.benchmark === item.id ? 'true' : undefined}><span>{item.name}</span></a>)}</div>;
          })}{!filtered.length && <p className={styles.empty}>No matching workloads.</p>}</nav>
        </aside>
        <div className={styles.main}>
          <label className={styles.mobileSelector}><span>Workload</span><select aria-label="Workload" value={benchmark.id} onChange={event => select({benchmark: event.target.value})}>{Object.entries(suites).map(([key, label]) => <optgroup key={key} label={label}>{data.cases.filter(item => item.suite === key).map(item => <option key={item.id} value={item.id}>{item.name}</option>)}</optgroup>)}</select></label>
          <div className={styles.titleRow}><div><p className={styles.eyebrow}>{suites[benchmark.suite]} / {benchmark.preset === 'L' ? 'L preset' : 'Full input'}</p><div className={styles.benchmarkHeading}><h2>{benchmark.title}</h2><BenchmarkNotes key={`${selection.machine}-${benchmark.id}`} benchmark={benchmark} /></div><p className={styles.subtitle}>{benchmark.suite === 'npbench' ? Object.entries(benchmark.parameters).map(([name, value]) => `${name} = ${value}`).join(' / ') : benchmark.arguments.length ? `Arguments: ${benchmark.arguments.join(' ')}` : 'Original application parameters'}</p></div><div className={styles.actions}><span role="status">{shareState}</span><button type="button" onClick={share} title="Copy benchmark link" aria-label="Copy benchmark link"><Icon icon={shareState === 'Link copied' ? Check : Link2} /></button><a href={`${assetBase}/results.json`} download title="Download results JSON" aria-label="Download results JSON"><Icon icon={Download} /></a></div></div>
          <div className={styles.chartControls}><div className={styles.segmented} role="group" aria-label="Chart metric"><button type="button" aria-pressed={mode === 'time'} onClick={() => setMode('time')}>Runtime</button><button type="button" aria-pressed={mode === 'speedup'} onClick={() => setMode('speedup')} disabled={!validTools.length}>Speedup</button></div><div className={styles.chartOptions}>{mode === 'speedup' && <label>Baseline <select aria-label="Speedup baseline" value={actualBaseline} onChange={event => setBaseline(event.target.value)}>{validTools.map(tool => <option key={tool} value={tool}>{benchmark.results[tool].label}</option>)}</select></label>}<label><input type="checkbox" checked={logarithmic} onChange={event => setLogarithmic(event.target.checked)} /> Log scale</label></div></div>
          <Chart benchmark={benchmark} mode={validTools.length ? mode : 'time'} baseline={actualBaseline} logarithmic={logarithmic} data={data} />
          <SourcePane benchmark={benchmark} tool={actualTool} view={selection.view} setTool={tool => select({tool})} setView={view => select({view})} data={data} />
        </div>
      </div>}
      <Methodology data={data} />
      <div className={styles.bottomLinks}><span>Original sweep {data.date}.{data.npbenchRefresh && ` Numba and MLP refreshed ${data.npbenchRefresh.date}.`}{data.histogramRefresh && ` Histogram refreshed ${data.histogramRefresh.date}.`} No pilot timings included.</span><a href={`${assetBase}/raw-results.json`} download>Download raw results</a></div>
    </main>
  </Layout>;
}
