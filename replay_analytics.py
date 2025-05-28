#!/usr/bin/env python3
"""
Advanced Analytics for Replay Data

This module provides tools for advanced analysis of replay data and mismatches,
including clustering, pattern recognition, and statistical analysis.
"""

import os
import sys
import json
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from collections import defaultdict, Counter
import jsonlines
import logging
from datetime import datetime
import argparse
from sklearn.cluster import DBSCAN, KMeans
from sklearn.decomposition import PCA
from sklearn.preprocessing import StandardScaler
from scipy.stats import ttest_ind, mannwhitneyu, pearsonr, spearmanr
import warnings

class ReplayAnalytics:
    """
    Advanced analytics for replay and validation data.
    
    This class provides methods for analyzing replay data to identify patterns in
    mismatches, perform statistical analysis, and visualize results.
    """
    
    def __init__(self):
        """Initialize the analytics object."""
        self.replay_frames = []
        self.validation_frames = []
        self.mismatches = []
        self.mismatch_frames = set()
        self.dataframe = None
        self.results = {}
    
    def load_data(self, replay_file, validation_file=None):
        """
        Load data from replay and validation files.
        
        Args:
            replay_file: Path to original replay file
            validation_file: Path to validation replay file (optional)
            
        Returns:
            Number of frames loaded
        """
        # Load replay frames
        self.replay_frames = self._load_jsonl(replay_file)
        logging.info(f"Loaded {len(self.replay_frames)} frames from {replay_file}")
        
        # Load validation frames if provided
        if validation_file:
            self.validation_frames = self._load_jsonl(validation_file)
            logging.info(f"Loaded {len(self.validation_frames)} frames from {validation_file}")
            
            # Find mismatches
            self.mismatches = self._find_mismatches()
            self.mismatch_frames = set(m['frame'] for m in self.mismatches)
            logging.info(f"Found {len(self.mismatches)} mismatches")
        
        # Convert to DataFrame for easier analysis
        self._prepare_dataframe()
        
        return len(self.replay_frames)
    
    def _load_jsonl(self, file_path):
        """Load data from a JSONL file."""
        frames = []
        try:
            with jsonlines.open(file_path) as reader:
                for line in reader:
                    frames.append(line)
        except Exception as e:
            logging.error(f"Error loading file {file_path}: {e}")
            return []
            
        # Sort frames by frame number if available
        if frames and 'frame' in frames[0]:
            frames.sort(key=lambda x: x.get('frame', 0))
            
        return frames
    
    def _find_mismatches(self):
        """Find mismatches between replay and validation frames."""
        mismatches = []
        
        # Create dictionaries mapping frame numbers to indices
        replay_map = {f.get('frame'): i for i, f in enumerate(self.replay_frames) if 'frame' in f}
        validation_map = {f.get('frame'): i for i, f in enumerate(self.validation_frames) if 'frame' in f}
        
        # Find common frames
        common_frames = set(replay_map.keys()) & set(validation_map.keys())
        
        # Compare frames
        for frame_num in common_frames:
            r_idx = replay_map[frame_num]
            v_idx = validation_map[frame_num]
            
            r_frame = self.replay_frames[r_idx]
            v_frame = self.validation_frames[v_idx]
            
            differences = []
            
            # Compare all fields
            for key in set(r_frame.keys()) & set(v_frame.keys()):
                if key == 'frame':
                    continue
                    
                if r_frame[key] != v_frame[key]:
                    differences.append({
                        'field': key,
                        'replay_value': r_frame[key],
                        'validation_value': v_frame[key]
                    })
            
            if differences:
                mismatches.append({
                    'frame': frame_num,
                    'replay_idx': r_idx,
                    'validation_idx': v_idx,
                    'differences': differences
                })
        
        return mismatches
    
    def _prepare_dataframe(self):
        """Convert replay frames to pandas DataFrame for analysis."""
        # Extract all unique keys from frames
        keys = set()
        for frame in self.replay_frames:
            keys.update(frame.keys())
        
        # Extract data
        data = []
        for frame in self.replay_frames:
            frame_num = frame.get('frame', 0)
            row = {'frame': frame_num, 'is_mismatch': frame_num in self.mismatch_frames}
            
            for key in keys:
                if key in frame:
                    row[key] = frame[key]
                else:
                    row[key] = None
            
            data.append(row)
        
        # Create DataFrame
        self.dataframe = pd.DataFrame(data)
        
        # Convert string columns that should be numeric
        for col in self.dataframe.columns:
            if col.startswith(('p1_', 'p2_', 'time_')) and self.dataframe[col].dtype == 'object':
                try:
                    self.dataframe[col] = pd.to_numeric(self.dataframe[col], errors='coerce')
                except Exception:
                    pass
        
        # Add derived columns
        if 'p1_x' in self.dataframe.columns and 'p2_x' in self.dataframe.columns:
            self.dataframe['distance_x'] = abs(self.dataframe['p1_x'] - self.dataframe['p2_x'])
        
        if 'p1_y' in self.dataframe.columns and 'p2_y' in self.dataframe.columns:
            self.dataframe['distance_y'] = abs(self.dataframe['p1_y'] - self.dataframe['p2_y'])
        
        # Add timing information
        self.dataframe['frame_diff'] = self.dataframe['frame'].diff()
    
    def run_analysis(self):
        """
        Run a comprehensive analysis of the replay data.
        
        Returns:
            Dictionary of analysis results
        """
        if self.dataframe is None or len(self.dataframe) == 0:
            logging.error("No data loaded for analysis")
            return {}
        
        results = {}
        
        # Basic statistics
        results['frame_count'] = len(self.dataframe)
        results['mismatch_count'] = len(self.mismatches)
        results['mismatch_rate'] = len(self.mismatches) / len(self.dataframe) if len(self.dataframe) > 0 else 0
        
        # Mismatch patterns
        results['mismatch_patterns'] = self.analyze_mismatch_patterns()
        
        # Mismatch clustering
        results['mismatch_clusters'] = self.cluster_mismatches()
        
        # Feature correlations with mismatches
        results['correlations'] = self.analyze_correlations()
        
        # Time series patterns
        results['time_patterns'] = self.analyze_time_patterns()
        
        # Store results
        self.results = results
        
        return results
    
    def analyze_mismatch_patterns(self):
        """
        Analyze patterns in mismatches.
        
        Returns:
            Dictionary of pattern analysis results
        """
        if not self.mismatches:
            return {}
        
        results = {}
        
        # Count mismatches by field
        field_counts = Counter()
        for mismatch in self.mismatches:
            for diff in mismatch.get('differences', []):
                field_counts[diff.get('field', 'unknown')] += 1
        
        results['field_counts'] = dict(field_counts)
        
        # Analyze timing patterns
        if len(self.mismatches) > 1:
            mismatch_frames = sorted(m['frame'] for m in self.mismatches)
            frame_diffs = [mismatch_frames[i] - mismatch_frames[i-1] for i in range(1, len(mismatch_frames))]
            
            results['frame_intervals'] = {
                'min': min(frame_diffs) if frame_diffs else None,
                'max': max(frame_diffs) if frame_diffs else None,
                'mean': np.mean(frame_diffs) if frame_diffs else None,
                'median': np.median(frame_diffs) if frame_diffs else None,
                'std': np.std(frame_diffs) if frame_diffs else None
            }
            
            # Check for periodicity
            if len(frame_diffs) > 5:
                from scipy import signal
                
                # Calculate autocorrelation
                try:
                    autocorr = signal.correlate(frame_diffs, frame_diffs, mode='full')
                    autocorr = autocorr[len(autocorr)//2:]
                    
                    # Find peaks in autocorrelation
                    peaks, _ = signal.find_peaks(autocorr, height=0.5*max(autocorr))
                    if len(peaks) > 1:
                        period = peaks[1]  # First peak is at lag 0
                        results['periodicity'] = {
                            'detected': True,
                            'period': period,
                            'confidence': autocorr[peaks[1]] / autocorr[0]
                        }
                    else:
                        results['periodicity'] = {'detected': False}
                except Exception as e:
                    logging.warning(f"Error in periodicity analysis: {e}")
                    results['periodicity'] = {'detected': False, 'error': str(e)}
        
        return results
    
    def cluster_mismatches(self):
        """
        Cluster mismatches to identify patterns.
        
        Returns:
            Dictionary of clustering results
        """
        if not self.mismatches or len(self.mismatches) < 5:
            return {'success': False, 'reason': 'Too few mismatches for clustering'}
        
        try:
            # Extract features for clustering
            features = []
            frame_nums = []
            
            for mismatch in self.mismatches:
                frame_num = mismatch.get('frame', 0)
                frame_nums.append(frame_num)
                
                # Find matching frame in DataFrame
                frame_data = self.dataframe[self.dataframe['frame'] == frame_num]
                
                if not frame_data.empty:
                    # Extract numerical features
                    row = frame_data.iloc[0]
                    feature_vec = []
                    
                    for col in ['p1_health', 'p2_health', 'p1_x', 'p1_y', 'p2_x', 'p2_y',
                               'distance_x', 'distance_y']:
                        if col in row and not pd.isna(row[col]):
                            feature_vec.append(float(row[col]))
                        else:
                            feature_vec.append(0.0)
                    
                    features.append(feature_vec)
            
            if not features:
                return {'success': False, 'reason': 'No valid features for clustering'}
            
            # Convert to numpy array
            X = np.array(features)
            
            # Standardize features
            scaler = StandardScaler()
            X_scaled = scaler.fit_transform(X)
            
            # Determine optimal number of clusters
            from sklearn.metrics import silhouette_score
            
            max_clusters = min(len(X_scaled) - 1, 10)  # At most 10 clusters
            if max_clusters < 2:
                return {'success': False, 'reason': 'Too few samples for clustering'}
            
            silhouette_scores = []
            for n_clusters in range(2, max_clusters + 1):
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore")
                    kmeans = KMeans(n_clusters=n_clusters, random_state=42)
                    cluster_labels = kmeans.fit_predict(X_scaled)
                    score = silhouette_score(X_scaled, cluster_labels)
                    silhouette_scores.append((n_clusters, score))
            
            # Select best number of clusters
            best_n_clusters = max(silhouette_scores, key=lambda x: x[1])[0]
            
            # Perform clustering
            kmeans = KMeans(n_clusters=best_n_clusters, random_state=42)
            cluster_labels = kmeans.fit_predict(X_scaled)
            
            # Analyze clusters
            clusters = {}
            for i, label in enumerate(cluster_labels):
                if label not in clusters:
                    clusters[label] = []
                
                clusters[label].append({
                    'frame': frame_nums[i],
                    'features': features[i]
                })
            
            # Perform PCA for visualization
            pca = PCA(n_components=2)
            X_pca = pca.fit_transform(X_scaled)
            
            # Prepare results
            cluster_stats = {}
            for label, items in clusters.items():
                frames = [item['frame'] for item in items]
                
                cluster_stats[label] = {
                    'count': len(items),
                    'frames': frames,
                    'center': kmeans.cluster_centers_[label].tolist(),
                    'frame_stats': {
                        'min': min(frames),
                        'max': max(frames),
                        'mean': np.mean(frames),
                        'std': np.std(frames)
                    }
                }
            
            return {
                'success': True,
                'method': 'kmeans',
                'n_clusters': best_n_clusters,
                'silhouette_scores': dict(silhouette_scores),
                'clusters': cluster_stats,
                'pca': {
                    'explained_variance': pca.explained_variance_ratio_.tolist(),
                    'points': X_pca.tolist(),
                    'labels': cluster_labels.tolist()
                }
            }
            
        except Exception as e:
            logging.error(f"Error in clustering: {e}")
            return {'success': False, 'error': str(e)}
    
    def analyze_correlations(self):
        """
        Analyze correlations between features and mismatches.
        
        Returns:
            Dictionary of correlation results
        """
        if self.dataframe is None or len(self.dataframe) == 0:
            return {}
        
        if 'is_mismatch' not in self.dataframe.columns:
            return {}
            
        results = {}
        
        # Select only numeric columns
        numeric_cols = self.dataframe.select_dtypes(include=[np.number]).columns.tolist()
        if 'is_mismatch' in numeric_cols:
            numeric_cols.remove('is_mismatch')
        
        # Calculate point-biserial correlation between features and mismatches
        correlations = {}
        for col in numeric_cols:
            if col == 'frame':
                continue
                
            try:
                # Get non-NaN values
                valid_data = self.dataframe[[col, 'is_mismatch']].dropna()
                if len(valid_data) < 5:
                    continue
                    
                # Calculate correlation
                corr, p_value = pearsonr(
                    valid_data[col], 
                    valid_data['is_mismatch'].astype(float)
                )
                
                correlations[col] = {
                    'correlation': corr,
                    'p_value': p_value,
                    'significant': p_value < 0.05
                }
            except Exception as e:
                logging.warning(f"Error calculating correlation for {col}: {e}")
        
        results['feature_correlations'] = correlations
        
        # Calculate mean values for mismatch vs non-mismatch frames
        comparison = {}
        for col in numeric_cols:
            if col == 'frame':
                continue
                
            try:
                # Get values for mismatch and non-mismatch frames
                mismatch_values = self.dataframe[self.dataframe['is_mismatch']][col].dropna()
                non_mismatch_values = self.dataframe[~self.dataframe['is_mismatch']][col].dropna()
                
                if len(mismatch_values) < 5 or len(non_mismatch_values) < 5:
                    continue
                
                # Calculate statistics
                stats = {
                    'mismatch_mean': mismatch_values.mean(),
                    'non_mismatch_mean': non_mismatch_values.mean(),
                    'mismatch_std': mismatch_values.std(),
                    'non_mismatch_std': non_mismatch_values.std(),
                    'difference': mismatch_values.mean() - non_mismatch_values.mean()
                }
                
                # Perform t-test
                try:
                    t_stat, p_value = ttest_ind(
                        mismatch_values, 
                        non_mismatch_values,
                        equal_var=False  # Welch's t-test
                    )
                    stats['t_stat'] = t_stat
                    stats['p_value'] = p_value
                    stats['significant'] = p_value < 0.05
                except Exception:
                    # Fallback to non-parametric test
                    try:
                        u_stat, p_value = mannwhitneyu(
                            mismatch_values, 
                            non_mismatch_values
                        )
                        stats['u_stat'] = u_stat
                        stats['p_value'] = p_value
                        stats['significant'] = p_value < 0.05
                    except Exception as e:
                        logging.warning(f"Error calculating statistical test for {col}: {e}")
                
                comparison[col] = stats
            except Exception as e:
                logging.warning(f"Error comparing values for {col}: {e}")
        
        results['feature_comparison'] = comparison
        
        return results
    
    def analyze_time_patterns(self):
        """
        Analyze temporal patterns in the replay data.
        
        Returns:
            Dictionary of time analysis results
        """
        if self.dataframe is None or len(self.dataframe) == 0:
            return {}
            
        results = {}
        
        # Check for consistent frame intervals
        if 'frame_diff' in self.dataframe.columns:
            frame_diffs = self.dataframe['frame_diff'].dropna()
            
            # Calculate statistics
            results['frame_interval'] = {
                'mean': frame_diffs.mean(),
                'median': frame_diffs.median(),
                'std': frame_diffs.std(),
                'min': frame_diffs.min(),
                'max': frame_diffs.max()
            }
            
            # Check for frame drops
            if len(frame_diffs) > 0:
                mean_diff = frame_diffs.mean()
                drops = frame_diffs[frame_diffs > 1.5 * mean_diff]
                
                if len(drops) > 0:
                    results['frame_drops'] = {
                        'count': len(drops),
                        'locations': self.dataframe.loc[drops.index, 'frame'].tolist(),
                        'severity': drops.tolist()
                    }
        
        # Analyze mismatch distribution over time
        if 'is_mismatch' in self.dataframe.columns and len(self.mismatches) > 0:
            # Split timeline into segments
            segment_count = min(20, len(self.dataframe) // 100 + 1)
            frame_min = self.dataframe['frame'].min()
            frame_max = self.dataframe['frame'].max()
            segment_size = (frame_max - frame_min) / segment_count
            
            segments = []
            for i in range(segment_count):
                start = frame_min + i * segment_size
                end = start + segment_size
                
                segment_frames = self.dataframe[(self.dataframe['frame'] >= start) & 
                                              (self.dataframe['frame'] < end)]
                
                mismatch_count = segment_frames['is_mismatch'].sum()
                total_count = len(segment_frames)
                
                segments.append({
                    'start_frame': start,
                    'end_frame': end,
                    'frame_count': total_count,
                    'mismatch_count': mismatch_count,
                    'mismatch_rate': mismatch_count / total_count if total_count > 0 else 0
                })
            
            results['time_segments'] = segments
            
            # Find segments with highest mismatch rates
            segments.sort(key=lambda x: x['mismatch_rate'], reverse=True)
            results['high_mismatch_segments'] = segments[:3]
        
        return results
    
    def visualize_results(self, output_dir=None):
        """
        Create visualizations of the analysis results.
        
        Args:
            output_dir: Directory to save visualization outputs
            
        Returns:
            True if visualizations were created, False otherwise
        """
        if not self.results or not self.dataframe.any().any():
            logging.warning("No results to visualize")
            return False
        
        # Create output directory if needed
        if output_dir:
            os.makedirs(output_dir, exist_ok=True)
        
        # Set Seaborn style
        sns.set(style="whitegrid")
        
        # Create visualizations
        self._visualize_mismatch_patterns(output_dir)
        self._visualize_clusters(output_dir)
        self._visualize_correlations(output_dir)
        self._visualize_time_patterns(output_dir)
        
        return True
    
    def _visualize_mismatch_patterns(self, output_dir=None):
        """Create visualizations of mismatch patterns."""
        if 'mismatch_patterns' not in self.results:
            return
            
        patterns = self.results['mismatch_patterns']
        
        # Visualize mismatch field counts
        if 'field_counts' in patterns and patterns['field_counts']:
            plt.figure(figsize=(10, 6))
            
            # Sort by count
            fields = sorted(patterns['field_counts'].items(), key=lambda x: x[1], reverse=True)
            field_names = [f[0] for f in fields]
            counts = [f[1] for f in fields]
            
            sns.barplot(x=counts, y=field_names)
            plt.title("Mismatch Counts by Field")
            plt.xlabel("Count")
            plt.ylabel("Field")
            plt.tight_layout()
            
            if output_dir:
                plt.savefig(os.path.join(output_dir, "mismatch_fields.png"))
                plt.close()
            else:
                plt.show()
        
        # Visualize frame intervals
        if 'frame_intervals' in patterns and patterns['frame_intervals']:
            # Create histogram of frame intervals
            mismatch_frames = sorted(m['frame'] for m in self.mismatches)
            frame_diffs = [mismatch_frames[i] - mismatch_frames[i-1] for i in range(1, len(mismatch_frames))]
            
            plt.figure(figsize=(10, 6))
            sns.histplot(frame_diffs, kde=True)
            plt.title("Distribution of Intervals Between Mismatches")
            plt.xlabel("Frames Between Mismatches")
            plt.ylabel("Count")
            plt.tight_layout()
            
            if output_dir:
                plt.savefig(os.path.join(output_dir, "mismatch_intervals.png"))
                plt.close()
            else:
                plt.show()
    
    def _visualize_clusters(self, output_dir=None):
        """Create visualizations of mismatch clusters."""
        if 'mismatch_clusters' not in self.results or not self.results['mismatch_clusters'].get('success', False):
            return
            
        clusters = self.results['mismatch_clusters']
        
        # Visualize PCA projection of clusters
        if 'pca' in clusters and 'points' in clusters['pca']:
            plt.figure(figsize=(10, 8))
            
            points = np.array(clusters['pca']['points'])
            labels = np.array(clusters['pca']['labels'])
            
            scatter = plt.scatter(points[:, 0], points[:, 1], c=labels, cmap='viridis', alpha=0.7)
            plt.colorbar(scatter, label='Cluster')
            
            # Plot cluster centers
            if 'clusters' in clusters:
                centers = []
                for label, data in clusters['clusters'].items():
                    if 'center' in data:
                        center_pca = PCA(n_components=2).fit_transform([data['center']])
                        centers.append((label, center_pca[0]))
                
                if centers:
                    center_labels = [c[0] for c in centers]
                    center_points = np.array([c[1] for c in centers])
                    plt.scatter(center_points[:, 0], center_points[:, 1], 
                               c=center_labels, cmap='viridis',
                               marker='*', s=200, edgecolor='black', linewidth=1)
            
            plt.title("PCA Projection of Mismatch Clusters")
            plt.xlabel(f"Principal Component 1 ({clusters['pca']['explained_variance'][0]:.2%} variance)")
            plt.ylabel(f"Principal Component 2 ({clusters['pca']['explained_variance'][1]:.2%} variance)")
            plt.tight_layout()
            
            if output_dir:
                plt.savefig(os.path.join(output_dir, "mismatch_clusters_pca.png"))
                plt.close()
            else:
                plt.show()
        
        # Visualize cluster distribution across frames
        if 'clusters' in clusters:
            plt.figure(figsize=(12, 6))
            
            # Plot frame numbers colored by cluster
            for label, data in clusters['clusters'].items():
                frames = data['frames']
                plt.scatter(frames, [label] * len(frames), alpha=0.7, label=f"Cluster {label}")
            
            plt.title("Mismatch Clusters Distribution Across Frames")
            plt.xlabel("Frame Number")
            plt.ylabel("Cluster")
            plt.yticks(list(clusters['clusters'].keys()))
            plt.grid(True, axis='y')
            plt.tight_layout()
            
            if output_dir:
                plt.savefig(os.path.join(output_dir, "cluster_distribution.png"))
                plt.close()
            else:
                plt.show()
    
    def _visualize_correlations(self, output_dir=None):
        """Create visualizations of feature correlations with mismatches."""
        if 'correlations' not in self.results or 'feature_correlations' not in self.results['correlations']:
            return
            
        correlations = self.results['correlations']['feature_correlations']
        
        if not correlations:
            return
            
        # Visualize correlations with mismatches
        plt.figure(figsize=(12, 6))
        
        # Sort by absolute correlation
        sorted_corrs = sorted(correlations.items(), key=lambda x: abs(x[1]['correlation']), reverse=True)
        features = [f[0] for f in sorted_corrs]
        corr_values = [f[1]['correlation'] for f in sorted_corrs]
        
        # Color by significance
        colors = ['green' if correlations[f]['significant'] else 'gray' for f in features]
        
        bars = plt.barh(features, corr_values, color=colors)
        plt.axvline(x=0, color='black', linestyle='-', linewidth=0.5)
        plt.title("Feature Correlations with Mismatches")
        plt.xlabel("Correlation Coefficient")
        plt.ylabel("Feature")
        
        # Add legend
        from matplotlib.lines import Line2D
        legend_elements = [
            Line2D([0], [0], marker='s', color='w', markerfacecolor='green', markersize=10, label='Significant (p<0.05)'),
            Line2D([0], [0], marker='s', color='w', markerfacecolor='gray', markersize=10, label='Not Significant')
        ]
        plt.legend(handles=legend_elements, loc='lower right')
        
        plt.tight_layout()
        
        if output_dir:
            plt.savefig(os.path.join(output_dir, "feature_correlations.png"))
            plt.close()
        else:
            plt.show()
        
        # Visualize comparison of feature values
        if 'feature_comparison' in self.results['correlations']:
            comparison = self.results['correlations']['feature_comparison']
            
            # Select top features with significant differences
            sig_features = [(f, data) for f, data in comparison.items() if data.get('significant', False)]
            sig_features.sort(key=lambda x: abs(x[1]['difference']), reverse=True)
            
            if sig_features:
                top_n = min(8, len(sig_features))
                top_features = sig_features[:top_n]
                
                plt.figure(figsize=(12, 8))
                
                # Prepare data for grouped bar chart
                features = [f[0] for f in top_features]
                mismatch_means = [f[1]['mismatch_mean'] for f in top_features]
                non_mismatch_means = [f[1]['non_mismatch_mean'] for f in top_features]
                
                x = np.arange(len(features))
                width = 0.35
                
                fig, ax = plt.subplots(figsize=(12, 8))
                rects1 = ax.bar(x - width/2, mismatch_means, width, label='Mismatch Frames')
                rects2 = ax.bar(x + width/2, non_mismatch_means, width, label='Non-Mismatch Frames')
                
                ax.set_title('Feature Values: Mismatch vs. Non-Mismatch Frames')
                ax.set_ylabel('Mean Value')
                ax.set_xticks(x)
                ax.set_xticklabels(features, rotation=45, ha='right')
                ax.legend()
                
                # Add value labels
                def autolabel(rects):
                    for rect in rects:
                        height = rect.get_height()
                        ax.annotate(f'{height:.2f}',
                                   xy=(rect.get_x() + rect.get_width() / 2, height),
                                   xytext=(0, 3),
                                   textcoords="offset points",
                                   ha='center', va='bottom')
                
                autolabel(rects1)
                autolabel(rects2)
                
                fig.tight_layout()
                
                if output_dir:
                    plt.savefig(os.path.join(output_dir, "feature_comparison.png"))
                    plt.close()
                else:
                    plt.show()
    
    def _visualize_time_patterns(self, output_dir=None):
        """Create visualizations of temporal patterns."""
        if 'time_patterns' not in self.results:
            return
            
        time_patterns = self.results['time_patterns']
        
        # Visualize mismatch distribution over time
        if 'time_segments' in time_patterns:
            segments = time_patterns['time_segments']
            
            if segments:
                plt.figure(figsize=(12, 6))
                
                # Extract segment data
                starts = [s['start_frame'] for s in segments]
                rates = [s['mismatch_rate'] * 100 for s in segments]  # Convert to percentage
                
                plt.bar(starts, rates, width=(segments[0]['end_frame'] - segments[0]['start_frame']))
                plt.title("Mismatch Rate Over Time")
                plt.xlabel("Frame Number")
                plt.ylabel("Mismatch Rate (%)")
                plt.grid(True, axis='y')
                plt.tight_layout()
                
                if output_dir:
                    plt.savefig(os.path.join(output_dir, "mismatch_time_distribution.png"))
                    plt.close()
                else:
                    plt.show()
        
        # Visualize frame intervals
        if 'frame_interval' in time_patterns:
            plt.figure(figsize=(10, 6))
            
            # Create histogram of frame intervals
            frame_diffs = self.dataframe['frame_diff'].dropna()
            
            sns.histplot(frame_diffs, kde=True)
            plt.axvline(x=time_patterns['frame_interval']['mean'], 
                       color='red', linestyle='--', 
                       label=f"Mean: {time_patterns['frame_interval']['mean']:.2f}")
            
            plt.title("Distribution of Frame Intervals")
            plt.xlabel("Frame Interval")
            plt.ylabel("Count")
            plt.legend()
            plt.tight_layout()
            
            if output_dir:
                plt.savefig(os.path.join(output_dir, "frame_intervals.png"))
                plt.close()
            else:
                plt.show()
        
        # Visualize frame drops
        if 'frame_drops' in time_patterns:
            drops = time_patterns['frame_drops']
            
            if drops and drops['count'] > 0:
                plt.figure(figsize=(12, 6))
                
                # Plot frame locations and severity
                plt.stem(drops['locations'], drops['severity'])
                plt.title(f"Frame Drops (Total: {drops['count']})")
                plt.xlabel("Frame Number")
                plt.ylabel("Frame Interval")
                plt.grid(True)
                plt.tight_layout()
                
                if output_dir:
                    plt.savefig(os.path.join(output_dir, "frame_drops.png"))
                    plt.close()
                else:
                    plt.show()
    
    def export_results(self, output_file):
        """
        Export analysis results to a file.
        
        Args:
            output_file: Path to output file (.json)
            
        Returns:
            True if export was successful, False otherwise
        """
        if not self.results:
            logging.warning("No results to export")
            return False
            
        try:
            # Create directory if needed
            os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
            
            # Add metadata
            export_data = {
                'timestamp': datetime.now().isoformat(),
                'replay_frames': len(self.replay_frames),
                'validation_frames': len(self.validation_frames),
                'mismatches': len(self.mismatches),
                'results': self.results
            }
            
            # Write to file
            with open(output_file, 'w') as f:
                json.dump(export_data, f, indent=2)
                
            logging.info(f"Exported results to {output_file}")
            return True
        except Exception as e:
            logging.error(f"Error exporting results: {e}")
            return False

def analyze_replay(replay_file, validation_file=None, output_dir=None, visualize=True):
    """
    Analyze a replay file and its validation.
    
    Args:
        replay_file: Path to replay file
        validation_file: Path to validation file (optional)
        output_dir: Directory to save outputs
        visualize: Whether to create visualizations
        
    Returns:
        Dictionary of analysis results
    """
    # Create output directory with timestamp if not specified
    if output_dir is None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_dir = f"replay_analysis_{timestamp}"
        
    os.makedirs(output_dir, exist_ok=True)
    
    # Initialize analytics
    analytics = ReplayAnalytics()
    
    # Load data
    logging.info(f"Loading replay data from {replay_file}")
    frame_count = analytics.load_data(replay_file, validation_file)
    
    if frame_count == 0:
        logging.error("Failed to load replay data")
        return {}
    
    # Run analysis
    logging.info("Running analysis...")
    results = analytics.run_analysis()
    
    # Create visualizations
    if visualize:
        logging.info("Creating visualizations...")
        analytics.visualize_results(output_dir)
    
    # Export results
    results_file = os.path.join(output_dir, "analysis_results.json")
    analytics.export_results(results_file)
    
    logging.info(f"Analysis complete. Results saved to {output_dir}")
    
    return results

def main():
    # Setup logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Advanced analytics for replay data')
    parser.add_argument('--replay', required=True, help='Path to replay file')
    parser.add_argument('--validation', help='Path to validation file')
    parser.add_argument('--output-dir', help='Directory to save outputs')
    parser.add_argument('--no-visualize', dest='visualize', action='store_false',
                        help='Disable visualization generation')
    
    args = parser.parse_args()
    
    # Run analysis
    results = analyze_replay(
        args.replay,
        args.validation,
        args.output_dir,
        args.visualize
    )
    
    return 0 if results else 1

if __name__ == "__main__":
    sys.exit(main()) 