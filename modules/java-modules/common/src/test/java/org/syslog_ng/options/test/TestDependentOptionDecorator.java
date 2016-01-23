package org.syslog_ng.options.test;

import org.junit.Before;
import org.junit.Test;
import org.syslog_ng.options.Option;
import org.syslog_ng.options.RequiredOptionDecorator;
import org.syslog_ng.options.DependentOptionDecorator;
import org.syslog_ng.options.StringOption;

public class TestDependentOptionDecorator extends TestOption {
	Option dependencyOption;
	Option dependentOption;

	@Before
	public void setUp() throws Exception {
		super.setUp();
		dependencyOption = new StringOption(owner, "string_dependency");
		RequiredOptionDecorator requiredOption = new RequiredOptionDecorator(new StringOption(owner, "required_dependent"));
		dependentOption = new DependentOptionDecorator(requiredOption, dependencyOption);
	}

	@Test
	public void dependentOptionWithDependencyShouldBeInitialized() {
		options.put("string_dependency", "test_dependency");
		options.put("required_dependent", "test_dependent");
		assertInitOptionSuccess(dependentOption);
	}

	@Test
	public void dependentOptionWithoutDependencyShouldNotBeInitialized() {
		options.put("required_dependent", "test_dependent");
		assertInitOptionFailed(dependentOption, "option string_dependency is a dependency of required_dependent");
	}
}
