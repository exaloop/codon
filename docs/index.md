---
title: Codon
sidebar_label: Overview
slug: /
hide_title: true
hide_table_of_contents: true
---

<header class="doc-intro">
  <p class="doc-kicker">Python simplicity, native performance.</p>
  <h1 class="codon-logo-heading" aria-label="Codon"><img class="codon-wordmark-light" src="/img/codon.svg" width="498" height="119" alt="" loading="eager" /><img class="codon-wordmark-dark" src="/img/codon-white.svg" width="498" height="119" alt="" loading="eager" /></h1>
  <p>A high-performance Python implementation that compiles to native machine code, with native multithreading and seamless Python interoperability.</p>
</header>

<figure class="codon-pipeline" aria-label="From Python source to native machine code" data-codon-pipeline>
  <div class="pipeline-regions">
    <div class="pipeline-region" data-pipeline-region="input"><span class="pipeline-region-label" id="pipeline-input-name"><span>Input</span></span></div>
    <div class="pipeline-region" data-pipeline-region="codon"><span class="pipeline-region-label" id="pipeline-compiler-name" role="img" aria-label="Codon"><img class="pipeline-compiler-logo codon-wordmark-light" src="/img/codon.svg" width="498" height="119" alt="" loading="eager" /><img class="pipeline-compiler-logo codon-wordmark-dark" src="/img/codon-white.svg" width="498" height="119" alt="" loading="eager" /></span></div>
    <div class="pipeline-region" data-pipeline-region="output"><span class="pipeline-region-label" id="pipeline-output-name"><span>Output</span></span></div>
  </div>
  <svg class="pipeline-connectors" aria-hidden="true" focusable="false"></svg>
  <div class="pipeline-notes" aria-live="polite" aria-atomic="true">
    <div class="pipeline-note-anchor">
    <p class="pipeline-note" id="pipeline-source-info" data-pipeline-info="source" hidden>Write familiar Python, then compile and run it with Codon.</p>
    <p class="pipeline-note" id="pipeline-ast-info" data-pipeline-info="ast" hidden>Python's syntax and semantics, represented as a tree that the compiler can analyze.</p>
    <p class="pipeline-note" id="pipeline-types-info" data-pipeline-info="types" hidden>Static type checking and inference enable native code generation, without requiring type annotations.</p>
    <p class="pipeline-note" id="pipeline-ir-info" data-pipeline-info="ir" hidden>A rich intermediate representation supports compiler analyses, transformations, and optimizations.</p>
    <p class="pipeline-note" id="pipeline-optimizations-info" data-pipeline-info="optimizations" hidden>General-purpose, Python-specific, and domain-specific optimizations turn high-level code into efficient native programs.</p>
    <p class="pipeline-note" id="pipeline-native-info" data-pipeline-info="native" hidden>LLVM lowers the optimized program to native machine code for your target hardware.</p>
    </div>
  </div>
  <div class="pipeline-labels">
    <div class="pipeline-label-group" role="group" aria-labelledby="pipeline-input-name">
      <a class="pipeline-label" id="pipeline-label-source" href="/start/usage/" data-pipeline-label="source" tabindex="-1"><small aria-hidden="true">00</small><strong>Python code</strong></a>
    </div>
    <div class="pipeline-compiler-group" role="group" aria-labelledby="pipeline-compiler-name">
      <a class="pipeline-label" id="pipeline-label-ast" href="/developers/compilation/#parsing" data-pipeline-label="ast" tabindex="-1"><small aria-hidden="true">01</small><strong>Syntax tree</strong></a>
      <a class="pipeline-label" id="pipeline-label-types" href="/developers/compilation/#type-checking" data-pipeline-label="types" tabindex="-1"><small aria-hidden="true">02</small><strong>Type checking</strong></a>
      <a class="pipeline-label" id="pipeline-label-ir" href="/developers/ir/" data-pipeline-label="ir" tabindex="-1"><small aria-hidden="true">03</small><strong>Codon IR</strong></a>
      <a class="pipeline-label" id="pipeline-label-optimizations" href="/developers/compilation/#codon-ir-optimization" data-pipeline-label="optimizations" tabindex="-1"><small aria-hidden="true">04</small><strong>Optimization</strong></a>
    </div>
    <div class="pipeline-label-group" role="group" aria-labelledby="pipeline-output-name">
      <a class="pipeline-label" id="pipeline-label-native" href="/developers/compilation/#code-generation" data-pipeline-label="native" tabindex="-1"><small aria-hidden="true">05</small><strong>Native code</strong></a>
    </div>
  </div>
  <div class="pipeline-flow">
    <svg class="pipeline-track" aria-hidden="true"><defs><marker id="pipeline-arrowhead" markerWidth="7" markerHeight="7" refX="6" refY="3.5" orient="auto"><path d="M1 1 L5 3.5 L1 6" /></marker><mask id="pipeline-flow-mask" maskUnits="userSpaceOnUse"></mask></defs><path class="pipeline-track-base" marker-end="url(#pipeline-arrowhead)" mask="url(#pipeline-flow-mask)" /><path class="pipeline-track-flow" mask="url(#pipeline-flow-mask)" /></svg>
    <div class="pipeline-stages">
      <a class="pipeline-stage pipeline-source pipeline-node" href="/start/usage/" data-pipeline-node="source" aria-labelledby="pipeline-label-source" aria-describedby="pipeline-source-info">
        <span class="pipeline-slab"><span class="pipeline-face">
          <span class="pipeline-port" aria-hidden="true"></span>
          <span class="pipeline-stage-body" aria-hidden="true"><span class="pipeline-editor-window">
            <span class="pipeline-editor-toolbar"><span class="pipeline-editor-tab"><img class="pipeline-python-logo" src="/img/python-logo.svg" width="20" height="20" loading="eager" alt="" /><span>add.py</span></span><span class="pipeline-editor-language">PY</span></span>
            <span class="pipeline-source-code"><span><b>def</b> <em>add</em>(a, b):</span><span>    <b>return</b> a + b</span><span> </span><span>result = <em>add</em>(<i>2</i>, <i>3</i>)</span></span>
            <span class="pipeline-editor-status"><span>UTF-8</span><span>Python</span></span>
          </span></span>
        </span></span>
      </a>
      <a class="pipeline-stage pipeline-node" href="/developers/compilation/#parsing" data-pipeline-node="ast" aria-labelledby="pipeline-label-ast" aria-describedby="pipeline-ast-info">
        <span class="pipeline-slab"><span class="pipeline-face">
          <span class="pipeline-port" aria-hidden="true"></span>
          <span class="pipeline-stage-body" aria-hidden="true"><span class="pipeline-expression"><span class="pipeline-operation"><span class="pipeline-symbol">+</span><small>binary op</small></span><svg class="pipeline-branches" viewBox="0 0 184 32"><path d="M92 0V10Q92 16 86 16H52Q46 16 46 22V32M92 10Q92 16 98 16H132Q138 16 138 22V32" /></svg><span class="pipeline-operand"><span>a</span><small>name</small></span><span class="pipeline-operand"><span>b</span><small>name</small></span></span></span>
        </span></span>
      </a>
      <a class="pipeline-stage pipeline-node" href="/developers/compilation/#type-checking" data-pipeline-node="types" aria-labelledby="pipeline-label-types" aria-describedby="pipeline-types-info">
        <span class="pipeline-slab"><span class="pipeline-face">
          <span class="pipeline-port" aria-hidden="true"></span>
          <span class="pipeline-stage-body" aria-hidden="true"><span class="pipeline-expression"><span class="pipeline-operation"><span class="pipeline-symbol">+</span><small>int</small></span><svg class="pipeline-branches" viewBox="0 0 184 32"><path d="M92 0V10Q92 16 86 16H52Q46 16 46 22V32M92 10Q92 16 98 16H132Q138 16 138 22V32" /></svg><span class="pipeline-operand"><span>a</span><small>int</small></span><span class="pipeline-operand"><span>b</span><small>int</small></span></span></span>
        </span></span>
      </a>
      <a class="pipeline-stage pipeline-node" href="/developers/ir/" data-pipeline-node="ir" aria-labelledby="pipeline-label-ir" aria-describedby="pipeline-ir-info">
        <span class="pipeline-slab"><span class="pipeline-face">
          <span class="pipeline-port" aria-hidden="true"></span>
          <span class="pipeline-stage-body" aria-hidden="true"><span class="pipeline-expression"><span class="pipeline-operation"><span class="pipeline-symbol">call</span><small>int.__add__</small></span><svg class="pipeline-branches" viewBox="0 0 184 32"><path d="M92 0V10Q92 16 86 16H52Q46 16 46 22V32M92 10Q92 16 98 16H132Q138 16 138 22V32" /></svg><span class="pipeline-operand"><span>%a</span><small>int</small></span><span class="pipeline-operand"><span>%b</span><small>int</small></span></span></span>
        </span></span>
      </a>
      <a class="pipeline-stage pipeline-node" href="/developers/compilation/#codon-ir-optimization" data-pipeline-node="optimizations" aria-labelledby="pipeline-label-optimizations" aria-describedby="pipeline-optimizations-info">
        <span class="pipeline-slab"><span class="pipeline-face">
          <span class="pipeline-port" aria-hidden="true"></span>
          <span class="pipeline-stage-body" aria-hidden="true"><span class="pipeline-expression"><span class="pipeline-operation"><span class="pipeline-symbol">add</span><small>i64</small></span><svg class="pipeline-branches" viewBox="0 0 184 32"><path d="M92 0V10Q92 16 86 16H52Q46 16 46 22V32M92 10Q92 16 98 16H132Q138 16 138 22V32" /></svg><span class="pipeline-operand"><span>%a</span><small>i64</small></span><span class="pipeline-operand"><span>%b</span><small>i64</small></span></span></span>
        </span></span>
      </a>
      <a class="pipeline-stage pipeline-native pipeline-node" href="/developers/compilation/#code-generation" data-pipeline-node="native" aria-labelledby="pipeline-label-native" aria-describedby="pipeline-native-info">
        <span class="pipeline-slab"><span class="pipeline-face">
          <svg class="pipeline-chip-pattern" viewBox="0 0 344 344" preserveAspectRatio="none" aria-hidden="true" focusable="false"><path class="pipeline-chip-frame" d="M38 20H306L324 38V306L306 324H38L20 306V38Z" /><g class="pipeline-chip-pins"></g><path class="pipeline-chip-traces" d="M62 44H282L300 62V282L282 300H62L44 282V62ZM114 44V34H100V20M230 44V34H244V20M300 114H310V100H324M300 230H310V244H324M114 300V310H100V324M230 300V310H244V324M44 114H34V100H20M44 230H34V244H20" /></svg>
          <span class="pipeline-port" aria-hidden="true"></span>
          <span class="pipeline-stage-body" aria-hidden="true"><span class="pipeline-cpu-content"><span class="pipeline-native-arch">x86-64</span><span class="pipeline-native-code"><span><b>lea</b> rax,</span><span>    [rdi + rsi]</span><span><b>ret</b></span></span><span class="pipeline-cpu-registers"><span>RAX</span><span>RDI</span><span>RSI</span></span></span></span>
        </span></span>
      </a>
    </div>
  </div>
</figure>

<div class="doc-discover">
  <section class="doc-start" aria-labelledby="goals">
    <div>
      <p class="doc-eyebrow">GET STARTED</p>
      <h2 id="goals">Build your first native program.</h2>
      <p class="doc-section-intro">Keep the Python syntax you know. Compile to native machine code, run across cores, and connect to the Python ecosystem.</p>
    </div>
    <div class="doc-start-actions">
      <a class="doc-primary-link" href="/start/install/"><svg class="doc-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#terminal" /></svg><span>Install Codon</span><svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></a>
      <a class="doc-text-link" href="/language/overview/"><svg class="doc-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#book-open" /></svg><span>Explore the language</span><svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></a>
      <a class="doc-text-link" href="/libraries/api/"><svg class="doc-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#braces" /></svg><span>Browse the API reference</span><svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></a>
    </div>
  </section>

  <section class="doc-use-section" aria-labelledby="uses">
    <p class="doc-eyebrow">PUT CODON TO WORK</p>
    <h2 id="uses">What will you build with Codon?</h2>
    <p class="doc-section-intro">Build standalone applications, explore parallel computing, or bring native code into your Python projects.</p>
    <div class="doc-use-grid">
      <a class="doc-use-case" href="/start/usage/">
        <svg class="doc-icon doc-use-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#terminal" /></svg>
        <h3>Compile, don't rewrite.</h3>
        <p>Turn compute-heavy Python into native executables for servers and edge devices, without moving to another language.</p>
        <span class="doc-use-link">Compile your first program<svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></span>
      </a>
      <a class="doc-use-case" href="/parallel/multithreading/">
        <svg class="doc-icon doc-use-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#cpu" /></svg>
        <h3>Put every core to work.</h3>
        <p>Spread independent work across CPU cores with native multithreading. No global interpreter lock holding it back.</p>
        <span class="doc-use-link">Explore multithreading<svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></span>
      </a>
      <a class="doc-use-case" href="/parallel/gpu/">
        <svg class="doc-icon doc-use-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#layers" /></svg>
        <h3>Take Python to the GPU.</h3>
        <p>Write GPU kernels in Python and bring massively parallel hardware to your data-intensive computations.</p>
        <span class="doc-use-link">Write a GPU kernel<svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></span>
      </a>
      <a class="doc-use-case" href="/libraries/numpy/">
        <svg class="doc-icon doc-use-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#grid" /></svg>
        <h3>Get more from NumPy.</h3>
        <p>Keep familiar array operations. Let the compiler fuse expressions and optimize the numerical work underneath.</p>
        <span class="doc-use-link">Meet Codon's NumPy<svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></span>
      </a>
      <a class="doc-use-case" href="/integrations/python/codon-from-python/">
        <svg class="doc-icon doc-use-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#zap" /></svg>
        <h3>Start with the bottleneck.</h3>
        <p>JIT-compile performance-sensitive functions inside an existing Python project. Keep the rest of your application in place.</p>
        <span class="doc-use-link">Add Codon to Python<svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></span>
      </a>
      <a class="doc-use-case" href="/integrations/python/extensions/">
        <svg class="doc-icon doc-use-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#blocks" /></svg>
        <h3>Build a faster Python module.</h3>
        <p>Package native code as a Python extension that other projects can import. No C or Cython required.</p>
        <span class="doc-use-link">Create an extension<svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></span>
      </a>
    </div>
  </section>

  <section class="doc-lab-invitation" aria-labelledby="doc-labs-title">
    <svg class="doc-icon doc-lab-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#flask" /></svg>
    <div>
      <p class="doc-eyebrow">CODON LABS</p>
      <h2 id="doc-labs-title">Go from reading to building.</h2>
      <p>Work through hands-on projects and explore what native Python can do.</p>
    </div>
    <a class="doc-text-link" href="/labs/"><span>Explore the labs</span><svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></a>
  </section>

  <section class="doc-compatibility" aria-labelledby="non-goals">
    <svg class="doc-icon" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#info" /></svg>
    <div>
      <h2 id="non-goals">Familiar Python. A different runtime.</h2>
      <p>Codon stays close to Python's syntax and semantics, but it is not a drop-in replacement for CPython. Some dynamic behavior and library interfaces differ. <a href="/integrations/python/python-from-codon/">Python interoperability</a> lets you keep using Python libraries alongside native code.</p>
      <a class="doc-text-link" href="/language/overview/#differences-with-python"><span>Read the compatibility guide</span><svg class="doc-icon doc-arrow" aria-hidden="true" focusable="false"><use href="/img/doc-icons.svg#arrow-right" /></svg></a>
    </div>
  </section>
</div>
